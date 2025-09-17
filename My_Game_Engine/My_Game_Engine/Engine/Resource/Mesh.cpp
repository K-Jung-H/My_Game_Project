#include "pch.h"
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
    if (mesh->HasNormals())  normals.resize(mesh->mNumVertices);
    if (mesh->HasTangentsAndBitangents()) tangents.resize(mesh->mNumVertices);
    if (mesh->HasTextureCoords(0)) uvs.resize(mesh->mNumVertices);
    if (mesh->HasVertexColors(0)) colors.resize(mesh->mNumVertices);

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        // Position
        positions[i] = XMFLOAT3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        // Normal
        if (mesh->HasNormals()) 
            normals[i] = XMFLOAT3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        

        // Tangent
        if (mesh->HasTangentsAndBitangents()) 
            tangents[i] = XMFLOAT3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
        

        // UV (0번 채널만)
        if (mesh->HasTextureCoords(0))
            uvs[i] = XMFLOAT2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        

        // Color (0번 채널만)
        if (mesh->HasVertexColors(0)) 
            colors[i] = XMFLOAT4(mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b, mesh->mColors[0][i].a);
    }

    // Index 복사
    indices.clear();
    indices.reserve(mesh->mNumFaces * 3);
    for (unsigned int f = 0; f < mesh->mNumFaces; f++) 
    {
        const aiFace& face = mesh->mFaces[f];
        for (unsigned int j = 0; j < face.mNumIndices; j++) 
            indices.push_back(face.mIndices[j]);

    }

    // Material Index
    materialIndex = mesh->mMaterialIndex;
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