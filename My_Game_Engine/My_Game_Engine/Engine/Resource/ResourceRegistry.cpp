#include "pch.h"
#include "ResourceRegistry.h"

LoadResult ResourceRegistry::Load(ResourceManager& manager, const std::string& path, std::string_view alias)
{
    LoadResult result;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenNormals);

    if (!scene) return result;

    for (unsigned int i = 0; i < scene->mNumMeshes; i++) 
    {
        auto mesh = std::make_shared<Mesh>();
        mesh->FromAssimp(scene->mMeshes[i]);
        mesh->SetId(mNextResourceID++);
        mesh->SetPath(path);
        mesh->SetAlias(path + "_mesh" + std::to_string(i));
        manager.Add(mesh);
        result.meshIds.push_back(mesh->GetId());
    }


    for (unsigned int i = 0; i < scene->mNumMaterials; i++) 
    {
        auto mat = std::make_shared<Material>();
        mat->FromAssimp(scene->mMaterials[i]);
        mat->SetId(mNextResourceID++);
        mat->SetPath(path);
        mat->SetAlias(path + "_mat" + std::to_string(i));
        manager.Add(mat);
        result.materialIds.push_back(mat->GetId());

        aiString texPath;
        if (scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) 
        {
            auto tex = std::make_shared<Texture>();
            tex->LoadFromFile(texPath.C_Str());
            tex->SetId(mNextResourceID++);
            tex->SetPath(texPath.C_Str());
            tex->SetAlias(path + "_tex" + std::to_string(i));
            manager.Add(tex);
            result.textureIds.push_back(tex->GetId());
        }
    }

    return result;
}