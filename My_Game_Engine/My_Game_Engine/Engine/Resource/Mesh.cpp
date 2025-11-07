#include "Mesh.h"
#include "GameEngine.h"
#include "Model.h"
#include "DX_Graphics/ResourceUtils.h"

static inline void CreateDefaultBufferWithUpload(
    const RendererContext& rc,
    const void* src, UINT sizeInBytes,
    D3D12_RESOURCE_FLAGS flags,
    D3D12_RESOURCE_STATES finalState,
    ComPtr<ID3D12Resource>& defaultBuf,
    ComPtr<ID3D12Resource>& uploadBuf)
{
    defaultBuf = ResourceUtils::CreateBufferResource(rc, (void*)src, sizeInBytes, D3D12_HEAP_TYPE_DEFAULT, flags, finalState, uploadBuf);
}

Mesh::Mesh() : Game_Resource(ResourceType::Mesh) {}

bool Mesh::LoadFromFile(std::string, const RendererContext&) { return true; }

void Mesh::BuildInterleavedBuffers()
{
    mVertexFlags = VertexFlags::None;
    if (!normals.empty())  mVertexFlags |= VertexFlags::HasNormal;
    if (!tangents.empty()) mVertexFlags |= VertexFlags::HasTangent;
    if (!uvs.empty())      mVertexFlags |= VertexFlags::HasUV0;
    if (!uv1s.empty())     mVertexFlags |= VertexFlags::HasUV1;
    if (!colors.empty())   mVertexFlags |= VertexFlags::HasColor0;

    const size_t vCount = positions.size();
    if (vCount == 0) return;

    mHotLayout = {};
    mHotLayout.stride = sizeof(XMFLOAT3) * 3;
    mHotLayout.position = { 0,  DXGI_FORMAT_R32G32B32_FLOAT, true };
    mHotLayout.normal = { 12, DXGI_FORMAT_R32G32B32_FLOAT, true };
    mHotLayout.tangent = { 24, DXGI_FORMAT_R32G32B32_FLOAT, true };

    mColdLayout = {};
    mColdLayout.stride = sizeof(XMFLOAT2) + sizeof(XMFLOAT2) + sizeof(XMFLOAT4);
    mColdLayout.uv0 = { 0,  DXGI_FORMAT_R32G32_FLOAT,       true };
    mColdLayout.uv1 = { 8,  DXGI_FORMAT_R32G32_FLOAT,       true };
    mColdLayout.color0 = { 16, DXGI_FORMAT_R32G32B32A32_FLOAT, true };

    mHotCPU.resize(mHotLayout.stride * vCount);
    mColdCPU.resize(mColdLayout.stride * vCount);
    if (!mHotCPU.empty())  memset(mHotCPU.data(), 0, mHotCPU.size());
    if (!mColdCPU.empty()) memset(mColdCPU.data(), 0, mColdCPU.size());

    for (size_t i = 0; i < vCount; ++i)
    {
        uint8_t* h = mHotCPU.data() + i * mHotLayout.stride;
        uint8_t* c = mColdCPU.data() + i * mColdLayout.stride;

        memcpy(h + mHotLayout.position.offset, &positions[i], sizeof(XMFLOAT3));

        if (HasFlag(mVertexFlags, VertexFlags::HasNormal))
        {
            if (i < normals.size()) memcpy(h + mHotLayout.normal.offset, &normals[i], sizeof(XMFLOAT3));
        }
        if (HasFlag(mVertexFlags, VertexFlags::HasTangent))
        {
            if (i < tangents.size()) memcpy(h + mHotLayout.tangent.offset, &tangents[i], sizeof(XMFLOAT3));
        }

        if (HasFlag(mVertexFlags, VertexFlags::HasUV0))
        {
            if (i < uvs.size()) memcpy(c + mColdLayout.uv0.offset, &uvs[i], sizeof(XMFLOAT2));
        }
        if (HasFlag(mVertexFlags, VertexFlags::HasUV1))
        {
            if (i < uv1s.size()) memcpy(c + mColdLayout.uv1.offset, &uv1s[i], sizeof(XMFLOAT2));
        }
        if (HasFlag(mVertexFlags, VertexFlags::HasColor0))
        {
            if (i < colors.size()) memcpy(c + mColdLayout.color0.offset, &colors[i], sizeof(XMFLOAT4));
        }
    }

    RendererContext rc = GameEngine::Get().Get_UploadContext();

    if (!mHotCPU.empty())
    {
        CreateDefaultBufferWithUpload(rc, mHotCPU.data(), (UINT)mHotCPU.size(),
            D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            mHotVB, mHotUpload);

        mHotVBV.BufferLocation = mHotVB->GetGPUVirtualAddress();
        mHotVBV.StrideInBytes = mHotLayout.stride;
        mHotVBV.SizeInBytes = (UINT)mHotCPU.size();
    }

    if (!mColdCPU.empty())
    {
        CreateDefaultBufferWithUpload(rc, mColdCPU.data(), (UINT)mColdCPU.size(),
            D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            mColdVB, mColdUpload);

        mColdVBV.BufferLocation = mColdVB->GetGPUVirtualAddress();
        mColdVBV.StrideInBytes = mColdLayout.stride;
        mColdVBV.SizeInBytes = (UINT)mColdCPU.size();
    }

    UploadIndexBuffer();
}

void Mesh::UploadIndexBuffer()
{
    if (indices.empty()) return;

    RendererContext rc = GameEngine::Get().Get_UploadContext();
    mIndexBuffer = ResourceUtils::CreateBufferResource(
        rc, indices.data(),
        (UINT)(sizeof(UINT) * indices.size()),
        D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE,
        D3D12_RESOURCE_STATE_INDEX_BUFFER, mIndexUpload);

    mIBV.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
    mIBV.SizeInBytes = (UINT)(sizeof(UINT) * indices.size());
    mIBV.Format = DXGI_FORMAT_R32_UINT;
}

void Mesh::SetAABB()
{
    if (positions.empty())
        return;

    if (!submeshes.empty())
    {
        XMVECTOR meshMin = XMVectorSet(+FLT_MAX, +FLT_MAX, +FLT_MAX, 0);
        XMVECTOR meshMax = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0);

        for (auto& sub : submeshes)
        {
            XMVECTOR subMin = XMVectorSet(+FLT_MAX, +FLT_MAX, +FLT_MAX, 0);
            XMVECTOR subMax = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0);

            for (UINT i = 0; i < sub.indexCount; ++i)
            {
                UINT idx = indices[sub.startIndexLocation + i];
                const XMFLOAT3& pos = positions[idx];
                XMVECTOR p = XMLoadFloat3(&pos);
                subMin = XMVectorMin(subMin, p);
                subMax = XMVectorMax(subMax, p);
            }
            XMVECTOR c = (subMin + subMax) * 0.5f;
            XMVECTOR e = (subMax - subMin) * 0.5f;
            XMStoreFloat3(&sub.localAABB.Center, c);
            XMStoreFloat3(&sub.localAABB.Extents, e);

            meshMin = XMVectorMin(meshMin, subMin);
            meshMax = XMVectorMax(meshMax, subMax);
        }

        XMVECTOR meshC = (meshMin + meshMax) * 0.5f;
        XMVECTOR meshE = (meshMax - meshMin) * 0.5f;
        XMStoreFloat3(&mLocalAABB.Center, meshC);
        XMStoreFloat3(&mLocalAABB.Extents, meshE);
    }
    else
    {
        XMVECTOR minv = XMVectorSet(+FLT_MAX, +FLT_MAX, +FLT_MAX, 0);
        XMVECTOR maxv = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0);
        for (auto& p3 : positions)
        {
            XMVECTOR p = XMLoadFloat3(&p3);
            minv = XMVectorMin(minv, p);
            maxv = XMVectorMax(maxv, p);
        }
        XMVECTOR c = (minv + maxv) * 0.5f;
        XMVECTOR e = (maxv - minv) * 0.5f;
        XMStoreFloat3(&mLocalAABB.Center, c);
        XMStoreFloat3(&mLocalAABB.Extents, e);
    }
}

void Mesh::FromAssimp(const aiMesh* mesh)
{
    positions.clear(); normals.clear(); tangents.clear();
    uvs.clear(); uv1s.clear(); colors.clear(); indices.clear(); submeshes.clear();

    const uint32_t vCount = mesh->mNumVertices;
    positions.resize(vCount);
    if (mesh->HasNormals())  normals.resize(vCount);
    if (mesh->HasTangentsAndBitangents()) tangents.resize(vCount);
    if (mesh->HasTextureCoords(0)) uvs.resize(vCount);
    if (mesh->HasTextureCoords(1)) uv1s.resize(vCount);
    if (mesh->HasVertexColors(0))  colors.resize(vCount);

    for (uint32_t i = 0; i < vCount; ++i)
    {
        positions[i] = XMFLOAT3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        if (!normals.empty())
        {
            const auto& n = mesh->mNormals[i];
            normals[i] = XMFLOAT3(n.x, n.y, n.z);
        }
        if (!tangents.empty())
        {
            const auto& t = mesh->mTangents[i];
            tangents[i] = XMFLOAT3(t.x, t.y, t.z);
        }
        if (!uvs.empty())
        {
            const auto& t0 = mesh->mTextureCoords[0][i];
            uvs[i] = XMFLOAT2((float)t0.x, (float)(1.0f - t0.y));
        }
        if (!uv1s.empty())
        {
            const auto& t1 = mesh->mTextureCoords[1][i];
            uv1s[i] = XMFLOAT2((float)t1.x, (float)t1.y);
        }
        if (!colors.empty())
        {
            const auto& c = mesh->mColors[0][i];
            colors[i] = XMFLOAT4(c.r, c.g, c.b, c.a);
        }
    }

    indices.reserve(mesh->mNumFaces * 3);
    for (uint32_t f = 0; f < mesh->mNumFaces; ++f)
    {
        const aiFace& face = mesh->mFaces[f];
        if (face.mNumIndices == 3)
        {
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }
    }

    Submesh s;
    s.indexCount = (UINT)indices.size();
    s.startIndexLocation = 0;
    s.baseVertexLocation = 0;
    s.materialId = Engine::INVALID_ID;
    submeshes.push_back(s);

    BuildInterleavedBuffers();
    SetAABB();
}

static inline XMFLOAT2 ReadFbxUV(FbxMesh* m, int polyIndex, int vertInPoly, int ctrlIndex, int setIdx)
{
    FbxStringList uvSetNames;
    m->GetUVSetNames(uvSetNames);
    if (setIdx >= uvSetNames.GetCount()) return XMFLOAT2(0, 0);

    const char* setName = uvSetNames.GetStringAt(setIdx);
    FbxVector2 uv;
    bool unmapped = false;
    m->GetPolygonVertexUV(polyIndex, vertInPoly, setName, uv, unmapped);
    return XMFLOAT2((float)uv[0], (float)(1.0f - uv[1]));
}

static inline bool ReadElemVec3_ByControlPoint(FbxMesh* m, FbxLayerElementTemplate<FbxVector4>* elem, std::vector<XMFLOAT3>& out)
{
    if (!elem) return false;
    if (elem->GetMappingMode() != FbxGeometryElement::eByControlPoint) return false;
    const auto& direct = elem->GetDirectArray();
    out.resize((size_t)m->GetControlPointsCount());
    for (int i = 0; i < m->GetControlPointsCount(); ++i)
    {
        FbxVector4 v = direct.GetAt(i);
        out[i] = XMFLOAT3((float)v[0], (float)v[1], (float)v[2]);
    }
    return true;
}

void Mesh::FromFbxSDK(FbxMesh* fbxMesh)
{
    positions.clear(); normals.clear(); tangents.clear();
    uvs.clear(); uv1s.clear(); colors.clear(); indices.clear(); submeshes.clear();

    const int cpCount = fbxMesh->GetControlPointsCount();
    positions.resize(cpCount);
    for (int i = 0; i < cpCount; ++i)
    {
        FbxVector4 p = fbxMesh->GetControlPointAt(i);
        positions[i] = XMFLOAT3((float)p[0], (float)p[1], (float)p[2]);
    }

    FbxGeometryElementNormal* nElem = fbxMesh->GetElementNormal(0);
    FbxGeometryElementTangent* tElem = fbxMesh->GetElementTangent(0);
    bool nOK = ReadElemVec3_ByControlPoint(fbxMesh, nElem, normals);
    bool tOK = ReadElemVec3_ByControlPoint(fbxMesh, tElem, tangents);

    const int polyCount = fbxMesh->GetPolygonCount();
    indices.reserve(polyCount * 3);
    if (!nOK) normals.resize(cpCount);
    if (!tOK) tangents.resize(cpCount);
    uvs.resize(cpCount);
    uv1s.resize(cpCount);

    for (int p = 0; p < polyCount; ++p)
    {
        const int vN = fbxMesh->GetPolygonSize(p);
        for (int v = 0; v < vN; ++v)
        {
            int ctrl = fbxMesh->GetPolygonVertex(p, v);
            indices.push_back((UINT)ctrl);

            if (!nOK && nElem)
            {
                FbxVector4 nv;
                fbxMesh->GetPolygonVertexNormal(p, v, nv);
                normals[ctrl] = XMFLOAT3((float)nv[0], (float)nv[1], (float)nv[2]);
            }
            if (!tOK && tElem)
            {
                FbxVector4 tv = tElem->GetDirectArray().GetAt(fbxMesh->GetTextureUVIndex(p, v));
                tangents[ctrl] = XMFLOAT3((float)tv[0], (float)tv[1], (float)tv[2]);
            }

            XMFLOAT2 uv0 = ReadFbxUV(fbxMesh, p, v, ctrl, 0);
            uvs[ctrl] = uv0;
            XMFLOAT2 uv1 = ReadFbxUV(fbxMesh, p, v, ctrl, 1);
            uv1s[ctrl] = uv1;
        }
    }

    Submesh s;
    s.indexCount = (UINT)indices.size();
    s.startIndexLocation = 0;
    s.baseVertexLocation = 0;
    s.materialId = Engine::INVALID_ID;
    submeshes.push_back(s);

    BuildInterleavedBuffers();
    SetAABB();
}


void Mesh::Bind(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    D3D12_VERTEX_BUFFER_VIEW views[2] = {};
    UINT count = 0;

    if (mHotVBV.BufferLocation && mHotVBV.SizeInBytes)
        views[count++] = mHotVBV;

    if (mColdVBV.BufferLocation && mColdVBV.SizeInBytes)
        views[count++] = mColdVBV;

    if (count) 
        cmdList->IASetVertexBuffers(0, count, views);
   

    if (mIBV.BufferLocation && mIBV.SizeInBytes) 
        cmdList->IASetIndexBuffer(&mIBV);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

Plane_Mesh::Plane_Mesh(float width, float height)
{
    GeneratePlane(width, height);
    BuildInterleavedBuffers();
    SetAABB();
}

void Plane_Mesh::GeneratePlane(float width, float height)
{
    positions.clear(); normals.clear(); tangents.clear(); uvs.clear(); uv1s.clear(); colors.clear(); indices.clear();

    const float hw = width * 0.5f;
    const float hh = height * 0.5f;

    positions = {
        {-hw, 0, -hh},
        { hw, 0, -hh},
        { hw, 0,  hh},
        {-hw, 0,  hh},
    };
    normals = { {0,1,0},{0,1,0},{0,1,0},{0,1,0} };
    tangents = { {1,0,0},{1,0,0},{1,0,0},{1,0,0} };

    uvs = { {0,0},{1,0},{1,1},{0,1} };


    uv1s = uvs;
    std::fill(uv1s.begin(), uv1s.end(), XMFLOAT2(0.0f, 0.0f));

    colors.resize(positions.size());
    std::fill(colors.begin(), colors.end(), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

    indices = { 0, 2, 1, 0, 3, 2 };

    submeshes.clear();

    Submesh s;
    s.indexCount = (UINT)indices.size();
    s.startIndexLocation = 0;
    s.baseVertexLocation = 0;
    s.materialId = Engine::INVALID_ID;
    submeshes.push_back(s);
}

void SkinnedMesh::FromAssimp(const aiMesh* mesh)
{
    Mesh::FromAssimp(mesh);

    bone_mapping_data.clear();
    if (!mesh->HasBones()) return;

    for (unsigned int b = 0; b < mesh->mNumBones; ++b)
    {
        aiBone* bone = mesh->mBones[b];
        std::string boneName = bone->mName.C_Str();
        for (unsigned int w = 0; w < bone->mNumWeights; ++w)
        {
            BoneMappingData m;
            m.boneName = boneName;
            m.vertexId = bone->mWeights[w].mVertexId;
            m.weight = bone->mWeights[w].mWeight;
            bone_mapping_data.push_back(std::move(m));
        }
    }

    mVertexFlags |= VertexFlags::Skinned;
}

void SkinnedMesh::FromFbxSDK(FbxMesh* fbxMesh)
{
    Mesh::FromFbxSDK(fbxMesh);

    bone_mapping_data.clear();
    int skinCount = fbxMesh->GetDeformerCount(FbxDeformer::eSkin);
    if (skinCount == 0) return;

    for (int s = 0; s < skinCount; ++s)
    {
        FbxSkin* skin = static_cast<FbxSkin*>(fbxMesh->GetDeformer(s, FbxDeformer::eSkin));
        if (!skin) continue;
        int clusterCount = skin->GetClusterCount();
        for (int c = 0; c < clusterCount; ++c)
        {
            FbxCluster* cl = skin->GetCluster(c);
            if (!cl || !cl->GetLink()) continue;
            std::string boneName = cl->GetLink()->GetName();
            int* idx = cl->GetControlPointIndices();
            double* w = cl->GetControlPointWeights();
            int cnt = cl->GetControlPointIndicesCount();
            for (int i = 0; i < cnt; ++i)
            {
                BoneMappingData m;
                m.boneName = boneName;
                m.vertexId = idx[i];
                m.weight = (float)w[i];
                bone_mapping_data.push_back(std::move(m));
            }
        }
    }

    mVertexFlags |= VertexFlags::Skinned;
}

void SkinnedMesh::Skinning_Skeleton_Bones(const Skeleton& skeleton)
{
    bone_vertex_data.clear();
    bone_vertex_data.resize(positions.size());

    for (auto& m : bone_mapping_data)
    {
        auto it = skeleton.NameToIndex.find(m.boneName);
        if (it == skeleton.NameToIndex.end()) continue;

        uint16_t boneIdx = (uint16_t)it->second;
        uint32_t vtx = m.vertexId;
        float w = m.weight;
        if (vtx >= bone_vertex_data.size()) continue;

        auto& v = bone_vertex_data[vtx];
        for (int s = 0; s < MAX_BONES_PER_VERTEX; ++s)
        {
            if (v.weights[s] == 0.0f)
            {
                v.boneIndices[s] = boneIdx;
                v.weights[s] = w;
                break;
            }
        }
    }
    bone_mapping_data.clear();

    for (auto& v : bone_vertex_data)
    {
        float sum = v.weights[0] + v.weights[1] + v.weights[2] + v.weights[3];
        if (sum > 0.0f)
        {
            float inv = 1.0f / sum;
            for (int i = 0; i < 4; ++i) v.weights[i] *= inv;
        }
    }

    const UINT vCount = (UINT)bone_vertex_data.size();
    if (vCount == 0) return;

    struct GPU_SkinData { uint16_t idx[4]; uint16_t pad[4]; float w[4]; };
    const UINT stride = sizeof(GPU_SkinData);
    std::vector<GPU_SkinData> blob(vCount);
    for (UINT i = 0; i < vCount; ++i)
    {
        memcpy(blob[i].idx, bone_vertex_data[i].boneIndices, sizeof(uint16_t) * 4);
        memcpy(blob[i].w, bone_vertex_data[i].weights, sizeof(float) * 4);
    }

    RendererContext rc = GameEngine::Get().Get_UploadContext();
    mSkinData = ResourceUtils::CreateBufferResource(
        rc, blob.data(), (UINT)(blob.size() * stride),
        D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, mSkinDataUpload);
}

void SkinnedMesh::Bind(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    D3D12_VERTEX_BUFFER_VIEW views[2] = {};
    UINT count = 0;
    if (mHotVBV.BufferLocation && mHotVBV.SizeInBytes)
        views[count++] = mHotVBV;
    if (mColdVBV.BufferLocation && mColdVBV.SizeInBytes)
        views[count++] = mColdVBV;

    if (count)
        cmdList->IASetVertexBuffers(0, count, views);

    if (mIBV.BufferLocation)
        cmdList->IASetIndexBuffer(&mIBV);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
void SkinnedMesh::CreatePreSkinnedOutputBuffer()
{
    if (!mHotVB || mSkinnedHotVB) return;

    const UINT bytes = mHotVBV.SizeInBytes;
    const RendererContext rc = GameEngine::Get().Get_UploadContext();

    mSkinnedHotVB = ResourceUtils::CreateBufferResource(
        rc, nullptr, bytes,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        mSkinnedHotUpload);

    mSkinnedHotVBV.BufferLocation = mSkinnedHotVB->GetGPUVirtualAddress();
    mSkinnedHotVBV.StrideInBytes = mHotVBV.StrideInBytes;
    mSkinnedHotVBV.SizeInBytes = bytes;
}
