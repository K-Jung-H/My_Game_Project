#include "pch.h"
#include "../DX_Graphics/ResourceUtils.h"
#include "GameEngine.h"
#include "Mesh.h"
#include "Model.h"

Mesh::Mesh() : Game_Resource()
{

}

bool Mesh::LoadFromFile(std::string_view path, const RendererContext& ctx)
{
	return true;
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
            uvs[i] = XMFLOAT2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);

        if (mesh->HasVertexColors(0))
            colors[i] = XMFLOAT4(mesh->mColors[0][i].r, mesh->mColors[0][i].g,
                mesh->mColors[0][i].b, mesh->mColors[0][i].a);
    }

    indices.clear();
    indices.reserve(mesh->mNumFaces * 3);
    for (unsigned int f = 0; f < mesh->mNumFaces; f++)
    {
        const aiFace& face = mesh->mFaces[f];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    materialIndex = mesh->mMaterialIndex;

    RendererContext rc = GameEngine::Get().Get_UploadContext();


    auto CreateVB = [&](const void* src, UINT stride, UINT count, 
        ComPtr<ID3D12Resource>& buffer, ComPtr<ID3D12Resource>& upload, 
        D3D12_VERTEX_BUFFER_VIEW& view, D3D12_RESOURCE_STATES finalState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
        {
            if (count == 0)
                return;

            buffer = ResourceUtils::CreateBufferResource(rc, (void*)src, stride * count, 
                D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE, finalState, upload);

            view.BufferLocation = buffer->GetGPUVirtualAddress();
            view.StrideInBytes = stride;
            view.SizeInBytes = stride * count;
        };


    CreateVB(positions.data(), sizeof(XMFLOAT3), (UINT)positions.size(), posBuffer, posUpload, posVBV);
    CreateVB(normals.data(), sizeof(XMFLOAT3), (UINT)normals.size(), normalBuffer, normalUpload, normalVBV);
    CreateVB(tangents.data(), sizeof(XMFLOAT3), (UINT)tangents.size(), tangentBuffer, tangentUpload, tangentVBV);
    CreateVB(uvs.data(), sizeof(XMFLOAT2), (UINT)uvs.size(), uvBuffer, uvUpload, uvVBV);
    CreateVB(colors.data(), sizeof(XMFLOAT4), (UINT)colors.size(), colorBuffer, colorUpload, colorVBV);


    if (!indices.empty())
    {
        indexBuffer = ResourceUtils::CreateBufferResource(rc, indices.data(), (UINT)(sizeof(UINT) * indices.size()), 
            D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_INDEX_BUFFER, indexUpload);

        indexView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
        indexView.SizeInBytes = (UINT)(sizeof(UINT) * indices.size());
        indexView.Format = DXGI_FORMAT_R32_UINT;
    }


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