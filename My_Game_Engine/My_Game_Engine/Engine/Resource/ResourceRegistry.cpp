#include "pch.h"
#include "ResourceRegistry.h"

LoadResult ResourceRegistry::Load(ResourceManager& manager, const std::string& path, std::string_view alias, const RendererContext& ctx)
{
    LoadResult result;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenNormals);

    if (!scene)
    {
        std::string errMsg = "[Assimp] Failed to load: " + path + "\nReason: " + importer.GetErrorString() + "\n";
        OutputDebugStringA(errMsg.c_str());
        return result;
    }

    // ---------------- Mesh ----------------
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

    // ---------------- Materials & Textures ----------------
    for (unsigned int i = 0; i < scene->mNumMaterials; i++)
    {
        auto mat = std::make_shared<Material>();
        mat->FromAssimp(scene->mMaterials[i]);
        mat->SetId(mNextResourceID++);
        mat->SetPath(path);
        mat->SetAlias(path + "_mat" + std::to_string(i));
        manager.Add(mat);
        result.materialIds.push_back(mat->GetId());

        // Load textures and assign to Material
        auto texIds = LoadMaterialTextures(manager, scene->mMaterials[i], path, mat, ctx);
        result.textureIds.insert(result.textureIds.end(), texIds.begin(), texIds.end());
    }

    return result;
}

std::vector<UINT> ResourceRegistry::LoadMaterialTextures(ResourceManager& manager, aiMaterial* material, const std::string& basePath, std::shared_ptr<Material>& mat, const RendererContext& ctx)
{
    std::vector<UINT> textureIds;

    static const std::vector<std::pair<aiTextureType, std::string>> textureTypes = 
    {
        { aiTextureType_DIFFUSE,           "diffuse" },
        { aiTextureType_NORMALS,           "normal" },
        { aiTextureType_DIFFUSE_ROUGHNESS, "roughness" },
        { aiTextureType_METALNESS,         "metallic" }
    };

    for (auto& [type, suffix] : textureTypes)
    {
        if (material->GetTextureCount(type) == 0)
            continue;

        aiString texPath;
        if (material->GetTexture(type, 0, &texPath) == AI_SUCCESS)
        {
            std::filesystem::path baseDir = std::filesystem::path(basePath).parent_path();
            std::filesystem::path texRel = std::filesystem::path(texPath.C_Str());
            std::filesystem::path fullPath = baseDir / texRel;

            if (auto existing = manager.GetByPath(fullPath.string()))
            {
                auto tex = std::static_pointer_cast<Texture>(existing);
                textureIds.push_back(tex->GetId());

                switch (type)
                {
                case aiTextureType_DIFFUSE:           mat->diffuseTexId = tex->GetId(); break;
                case aiTextureType_NORMALS:           mat->normalTexId = tex->GetId(); break;
                case aiTextureType_DIFFUSE_ROUGHNESS: mat->roughnessTexId = tex->GetId(); break;
                case aiTextureType_METALNESS:         mat->metallicTexId = tex->GetId(); break;
                }

                continue;
            }

            auto tex = std::make_shared<Texture>();

            if (!tex->LoadFromFile(fullPath.string(), ctx))
                continue;

            tex->SetId(mNextResourceID++);
            tex->SetPath(fullPath.string());
            tex->SetAlias(basePath + "_tex_" + suffix);
            manager.Add(tex);

            textureIds.push_back(tex->GetId());

            switch (type)
            {
            case aiTextureType_DIFFUSE:           mat->diffuseTexId = tex->GetId(); break;
            case aiTextureType_NORMALS:           mat->normalTexId = tex->GetId(); break;
            case aiTextureType_DIFFUSE_ROUGHNESS: mat->roughnessTexId = tex->GetId(); break;
            case aiTextureType_METALNESS:         mat->metallicTexId = tex->GetId(); break;
            }
        }
    }

    return textureIds;
}
