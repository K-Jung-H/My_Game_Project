#include "Mesh.h"
#include "GameEngine.h"
#include "Model.h"
#include "DX_Graphics/ResourceUtils.h"

Mesh::Mesh() : Game_Resource(ResourceType::Mesh)
{

}

bool Mesh::LoadFromFile(std::string path, const RendererContext& ctx)
{
	return true;
}

void Mesh::UploadToGPU()
{
    const size_t vCount = positions.size();

    RendererContext rc = GameEngine::Get().Get_UploadContext();

    auto CreateVB = [&](const void* src, UINT stride, UINT count,
        ComPtr<ID3D12Resource>& buffer, ComPtr<ID3D12Resource>& upload,
        D3D12_VERTEX_BUFFER_VIEW& view,
        D3D12_RESOURCE_STATES finalState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
        {
            buffer = ResourceUtils::CreateBufferResource(rc, (void*)src, stride * count,
                D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE, finalState, upload);

            view.BufferLocation = buffer->GetGPUVirtualAddress();
            view.StrideInBytes = stride;
            view.SizeInBytes = stride * count;
        };

    if (!positions.empty())
        CreateVB(positions.data(), sizeof(XMFLOAT3), (UINT)vCount, posBuffer, posUpload, posVBV);
    if (!normals.empty())
        CreateVB(normals.data(), sizeof(XMFLOAT3), (UINT)vCount, normalBuffer, normalUpload, normalVBV);
    if (!tangents.empty())
        CreateVB(tangents.data(), sizeof(XMFLOAT3), (UINT)vCount, tangentBuffer, tangentUpload, tangentVBV);
    if (!uvs.empty())
        CreateVB(uvs.data(), sizeof(XMFLOAT2), (UINT)vCount, uvBuffer, uvUpload, uvVBV);
    if (!colors.empty())
        CreateVB(colors.data(), sizeof(XMFLOAT4), (UINT)vCount, colorBuffer, colorUpload, colorVBV);

    if (!indices.empty())
    {
        indexBuffer = ResourceUtils::CreateBufferResource(rc, indices.data(),
            (UINT)(sizeof(UINT) * indices.size()),
            D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE,
            D3D12_RESOURCE_STATE_INDEX_BUFFER, indexUpload);

        indexView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
        indexView.SizeInBytes = (UINT)(sizeof(UINT) * indices.size());
        indexView.Format = DXGI_FORMAT_R32_UINT;
    }
}

void Mesh::FromAssimp(const aiMesh* mesh)
{
    positions.resize(mesh->mNumVertices);
    if (mesh->HasNormals())               normals.resize(mesh->mNumVertices);
    if (mesh->HasTangentsAndBitangents()) tangents.resize(mesh->mNumVertices);
    if (mesh->HasTextureCoords(0))        uvs.resize(mesh->mNumVertices);
    if (mesh->HasVertexColors(0))         colors.resize(mesh->mNumVertices);

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        positions[i] = XMFLOAT3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        if (mesh->HasNormals())
            normals[i] = XMFLOAT3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

        if (mesh->HasTangentsAndBitangents())
            tangents[i] = XMFLOAT3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);

        if (mesh->HasTextureCoords(0))
            uvs[i] = XMFLOAT2(mesh->mTextureCoords[0][i].x, 1.0f - mesh->mTextureCoords[0][i].y);

        if (mesh->HasVertexColors(0))
            colors[i] = XMFLOAT4(mesh->mColors[0][i].r, mesh->mColors[0][i].g,
                mesh->mColors[0][i].b, mesh->mColors[0][i].a);
    }

    const size_t vCount = positions.size();
    if (normals.empty())   normals.assign(vCount, XMFLOAT3(0, 0, 1));
    if (tangents.empty())  tangents.assign(vCount, XMFLOAT3(1, 0, 0));
    if (uvs.empty())       uvs.assign(vCount, XMFLOAT2(0, 0));
    if (colors.empty())    colors.assign(vCount, XMFLOAT4(1, 1, 1, 1));

    indices.clear();
    indices.reserve(mesh->mNumFaces * 3);
    for (unsigned int f = 0; f < mesh->mNumFaces; f++)
    {
        const aiFace& face = mesh->mFaces[f];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    UploadToGPU();
}

void Mesh::FromFbxSDK(FbxMesh* fbxMesh)
{
    if (!fbxMesh) return;

    int polyCount = fbxMesh->GetPolygonCount();
    int vertexCount = fbxMesh->GetPolygonVertexCount(); // polygon-vertex

    positions.resize(vertexCount);
    normals.resize(vertexCount, XMFLOAT3(0, 0, 1));
    tangents.resize(vertexCount, XMFLOAT3(1, 0, 0));
    uvs.resize(vertexCount, XMFLOAT2(0, 0));
    colors.resize(vertexCount, XMFLOAT4(1, 1, 1, 1));
    indices.resize(vertexCount);

    // UV Element
    FbxGeometryElementUV* uvElem = (fbxMesh->GetElementUVCount() > 0) ? fbxMesh->GetElementUV(0) : nullptr;
    FbxGeometryElement::EMappingMode uvMapMode = uvElem ? uvElem->GetMappingMode() : FbxGeometryElement::eNone;
    FbxGeometryElement::EReferenceMode uvRefMode = uvElem ? uvElem->GetReferenceMode() : FbxGeometryElement::eDirect;

    // Tangent Element
    FbxGeometryElementTangent* tangentElem = (fbxMesh->GetElementTangentCount() > 0) ? fbxMesh->GetElementTangent(0) : nullptr;

    // Color Element
    FbxGeometryElementVertexColor* colorElem = (fbxMesh->GetElementVertexColorCount() > 0) ? fbxMesh->GetElementVertexColor(0) : nullptr;

    int idx = 0;
    for (int p = 0; p < polyCount; p++)
    {
        int vCountInPoly = fbxMesh->GetPolygonSize(p);
        for (int v = 0; v < vCountInPoly; v++)
        {
            int ctrlPointIndex = fbxMesh->GetPolygonVertex(p, v);

            // --- Position ---
            FbxVector4 pos = fbxMesh->GetControlPointAt(ctrlPointIndex);
            positions[idx] = XMFLOAT3((float)pos[0], (float)pos[1], (float)pos[2]);

            // --- Normal ---
            FbxVector4 n;
            if (fbxMesh->GetPolygonVertexNormal(p, v, n))
                normals[idx] = XMFLOAT3((float)n[0], (float)n[1], (float)n[2]);

            // --- Tangent ---
            if (tangentElem && tangentElem->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
            {
                int tIndex = (tangentElem->GetReferenceMode() == FbxGeometryElement::eDirect) ? idx : tangentElem->GetIndexArray().GetAt(idx);
                FbxVector4 t = tangentElem->GetDirectArray().GetAt(tIndex);
                tangents[idx] = XMFLOAT3((float)t[0], (float)t[1], (float)t[2]);
            }

            // --- UV ---
            if (uvElem)
            {
                FbxVector2 uv(0, 0);
                if (uvMapMode == FbxGeometryElement::eByControlPoint)
                {
                    int uvIdx = (uvRefMode == FbxGeometryElement::eDirect)
                        ? ctrlPointIndex
                        : uvElem->GetIndexArray().GetAt(ctrlPointIndex);
                    uv = uvElem->GetDirectArray().GetAt(uvIdx);
                }
                else if (uvMapMode == FbxGeometryElement::eByPolygonVertex)
                {
                    int uvIdx = (uvRefMode == FbxGeometryElement::eDirect)
                        ? idx
                        : uvElem->GetIndexArray().GetAt(idx);
                    uv = uvElem->GetDirectArray().GetAt(uvIdx);
                }
                uvs[idx] = XMFLOAT2((float)uv[0], 1.0f - (float)uv[1]);
            }

            // --- Color ---
            if (colorElem && colorElem->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
            {
                int cIdx = (colorElem->GetReferenceMode() == FbxGeometryElement::eDirect)
                    ? idx
                    : colorElem->GetIndexArray().GetAt(idx);
                FbxColor c = colorElem->GetDirectArray().GetAt(cIdx);
                colors[idx] = XMFLOAT4((float)c.mRed, (float)c.mGreen, (float)c.mBlue, (float)c.mAlpha);
            }

            // --- Index ---
            // flatten ¡æ index == vertex
            indices[idx] = idx; 
            idx++;
        }
    }

    UploadToGPU();
}

void Mesh::Bind(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    std::vector<D3D12_VERTEX_BUFFER_VIEW> views;
    if (posVBV.BufferLocation)    views.push_back(posVBV);
    if (normalVBV.BufferLocation) views.push_back(normalVBV);
    if (tangentVBV.BufferLocation)views.push_back(tangentVBV);
    if (uvVBV.BufferLocation)     views.push_back(uvVBV);
    if (colorVBV.BufferLocation)  views.push_back(colorVBV);

    if (!views.empty())
        cmdList->IASetVertexBuffers(0, (UINT)views.size(), views.data());

    if (indexView.BufferLocation)
        cmdList->IASetIndexBuffer(&indexView);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

Plane_Mesh::Plane_Mesh(float width, float height)
{
    GeneratePlane(width, height);
    UploadToGPU();
}

void Plane_Mesh::GeneratePlane(float width, float depth) 
{
    float halfWidth = width / 2.0f;
    float halfDepth = depth / 2.0f; 

    positions = {
        { -halfWidth, 0.0f, -halfDepth }, 
        { -halfWidth, 0.0f,  halfDepth }, 
        {  halfWidth, 0.0f,  halfDepth }, 
        {  halfWidth, 0.0f, -halfDepth } 
    };

    normals = {
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f }
    };

    tangents = {
        { 1.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f }
    };

        uvs = {
        { 0.0f, 1.0f },
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f } 
    };

    colors = {
        { 1.0f, 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f, 1.0f }
    };

    indices = {
        0, 1, 2, 
        0, 2, 3  
    };

    Submesh submesh;
    submesh.indexCount = static_cast<UINT>(indices.size());
    submesh.startIndexLocation = 0;
    submesh.baseVertexLocation = 0;
    submesh.materialId = Engine::INVALID_ID;
    submeshes.push_back(submesh);
}


void SkinnedMesh::FromAssimp(const aiMesh* mesh)
{
    Mesh::FromAssimp(mesh);

    bone_mapping_data.clear();

    if (!mesh->HasBones())
        return; 

    for (unsigned int bone = 0; bone < mesh->mNumBones; bone++)
    {
        aiBone* aBone = mesh->mBones[bone];
        std::string boneName = aBone->mName.C_Str();

        for (unsigned int w = 0; w < aBone->mNumWeights; w++)
        {
            BoneMappingData mapping;
            mapping.boneName = boneName;
            mapping.vertexId = aBone->mWeights[w].mVertexId;
            mapping.weight = aBone->mWeights[w].mWeight;

            bone_mapping_data.push_back(std::move(mapping));
        }
    }
}

void SkinnedMesh::FromFbxSDK(FbxMesh* fbxMesh)
{
    Mesh::FromFbxSDK(fbxMesh);

    bone_mapping_data.clear();

    int skinCount = fbxMesh->GetDeformerCount(FbxDeformer::eSkin);
    if (skinCount == 0)
        return; 

    for (int s = 0; s < skinCount; s++)
    {
        FbxSkin* skin = static_cast<FbxSkin*>(fbxMesh->GetDeformer(s, FbxDeformer::eSkin));
        if (!skin) continue;

        int clusterCount = skin->GetClusterCount();
        for (int c = 0; c < clusterCount; c++)
        {
            FbxCluster* cluster = skin->GetCluster(c);
            if (!cluster) continue;

            std::string boneName = cluster->GetLink()->GetName();

            int* indices = cluster->GetControlPointIndices();
            double* weights = cluster->GetControlPointWeights();
            int idxCount = cluster->GetControlPointIndicesCount();

            for (int i = 0; i < idxCount; i++)
            {
                BoneMappingData mapping;
                mapping.boneName = boneName;
                mapping.vertexId = indices[i];
                mapping.weight = static_cast<float>(weights[i]);

                bone_mapping_data.push_back(std::move(mapping));
            }
        }
    }
}


void SkinnedMesh::Skinning_Skeleton_Bones(const Skeleton& skeleton)
{
    if (is_skinned_bones != false)
        return;
        
    bone_vertex_data.clear();
    bone_vertex_data.resize(positions.size());

    for (auto& mapping : bone_mapping_data)
    {
        auto it = skeleton.NameToIndex.find(mapping.boneName);
        if (it == skeleton.NameToIndex.end())
            continue; 

        int boneIndex = it->second;
        UINT vertexId = mapping.vertexId;
        float weight = mapping.weight;

        if (vertexId >= bone_vertex_data.size())
            continue;

        auto& vbd = bone_vertex_data[vertexId];


        for (int slot = 0; slot < MAX_BONES_PER_VERTEX; slot++)
        {
            if (vbd.weights[slot] == 0.0f)
            {
                vbd.boneIndices[slot] = boneIndex;
                vbd.weights[slot] = weight;
                break;
            }
        }
    }

    bone_mapping_data.clear();
}