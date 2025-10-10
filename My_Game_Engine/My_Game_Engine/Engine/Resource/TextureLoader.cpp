#include "TextureLoader.h"
#include "ResourceRegistry.h"

std::vector<UINT> TextureLoader::LoadFromAssimp(
    ResourceManager& manager,
    aiMaterial* material,
    const std::string& basePath,
    std::shared_ptr<Material>& mat,
    const RendererContext& ctx)
{
    std::vector<UINT> textureIds;

    static const std::vector<std::pair<aiTextureType, std::string>> textureTypes =
    {
        { aiTextureType_DIFFUSE, "diffuse" },
        { aiTextureType_NORMALS, "normal" },
        { aiTextureType_DIFFUSE_ROUGHNESS, "roughness" },
        { aiTextureType_METALNESS, "metallic" }
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

            auto tex = manager.GetByPath<Texture>(fullPath.string());
            if (!tex)
            {
                tex = std::make_shared<Texture>();
                if (!tex->LoadFromFile(fullPath.string(), ctx))
                    continue;

                tex->SetId(ResourceRegistry::GenerateID());
                tex->SetPath(fullPath.string());
                tex->SetAlias(basePath + "_tex_" + suffix);
                manager.Add(tex);
            }

            textureIds.push_back(tex->GetId());

            switch (type)
            {
            case aiTextureType_DIFFUSE:
                mat->diffuseTexId = tex->GetId();
                mat->diffuseTexSlot = tex->GetSlot();
                break;
            case aiTextureType_NORMALS:
                mat->normalTexId = tex->GetId();
                mat->normalTexSlot = tex->GetSlot();
                break;
            case aiTextureType_DIFFUSE_ROUGHNESS:
                mat->roughnessTexId = tex->GetId();
                mat->roughnessTexSlot = tex->GetSlot();
                break;
            case aiTextureType_METALNESS:
                mat->metallicTexId = tex->GetId();
                mat->metallicTexSlot = tex->GetSlot();
                break;
            }
        }
    }
    return textureIds;
}

std::vector<UINT> TextureLoader::LoadFromFbx(
    ResourceManager& manager,
    FbxSurfaceMaterial* fbxMat,
    const std::string& basePath,
    std::shared_ptr<Material>& mat,
    const RendererContext& ctx)
{
    std::vector<UINT> textureIds;

    auto LoadTex = [&](const char* propName, UINT& texId, UINT& texSlot)
        {
            FbxProperty prop = fbxMat->FindProperty(propName);
            if (!prop.IsValid()) return;

            int texCount = prop.GetSrcObjectCount<FbxFileTexture>();
            if (texCount <= 0) return;

            FbxFileTexture* tex = prop.GetSrcObject<FbxFileTexture>(0);
            if (!tex) return;

            std::filesystem::path baseDir = std::filesystem::path(basePath).parent_path();
            std::filesystem::path texPath = tex->GetFileName();

            std::filesystem::path relPath = texPath.filename();
            std::filesystem::path fullPath = baseDir / relPath;

            if (auto existTex = manager.GetByPath<Texture>(fullPath.string()))
            {
                texId = existTex->GetId();
                texSlot = existTex->GetSlot();
                textureIds.push_back(texId);
                return;
            }

            auto newTex = std::make_shared<Texture>();
            if (!newTex->LoadFromFile(fullPath.string(), ctx))
                return;

            newTex->SetId(ResourceRegistry::GenerateID());
            newTex->SetPath(fullPath.string());
            newTex->SetAlias(fullPath.stem().string());
            manager.Add(newTex);

            texId = newTex->GetId();
            texSlot = newTex->GetSlot();
            textureIds.push_back(texId);
        };

    // FBX 기본 속성 이름 (DX12 PBR 기준)
    LoadTex(FbxSurfaceMaterial::sDiffuse, mat->diffuseTexId, mat->diffuseTexSlot);
    LoadTex(FbxSurfaceMaterial::sNormalMap, mat->normalTexId, mat->normalTexSlot);
    LoadTex(FbxSurfaceMaterial::sSpecular, mat->roughnessTexId, mat->roughnessTexSlot);
    LoadTex(FbxSurfaceMaterial::sReflection, mat->metallicTexId, mat->metallicTexSlot);

    return textureIds;
}
