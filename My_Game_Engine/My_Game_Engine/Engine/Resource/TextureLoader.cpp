#include "TextureLoader.h"
#include "GameEngine.h"
#include "MetaIO.h"

std::vector<UINT> TextureLoader::LoadFromAssimp(
    const RendererContext& ctx,
    aiMaterial* material,
    const std::string& basePath,
    std::shared_ptr<Material>& mat)
{
    ResourceSystem* rs = GameEngine::Get().GetResourceSystem();
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

            auto tex = rs->GetByPath<Texture>(fullPath.string());
            if (!tex)
            {
                tex = std::make_shared<Texture>();
                if (!tex->LoadFromFile(fullPath.string(), ctx))
                {
                    OutputDebugStringA(("[TextureLoader] Failed to load texture: " + fullPath.string() + "\n").c_str());
                    continue;
                }

                tex->SetAlias(fullPath.stem().string());
                tex->SetPath(fullPath.string());
                rs->RegisterResource(tex);
            }
            else
            {
                OutputDebugStringA(("[TextureLoader] Reused cached texture: " + fullPath.string() + "\n").c_str());
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
    const RendererContext& ctx,
    FbxSurfaceMaterial* fbxMat,
    const std::string& basePath,
    std::shared_ptr<Material>& mat)
{
    ResourceSystem* rs = GameEngine::Get().GetResourceSystem();
    std::vector<UINT> textureIds;

    auto LoadTex = [&](const char* propName, UINT& texId, UINT& texSlot)
        {
            FbxProperty prop = fbxMat->FindProperty(propName);
            if (!prop.IsValid()) return;

            int texCount = prop.GetSrcObjectCount<FbxFileTexture>();
            if (texCount <= 0) return;

            FbxFileTexture* tex = prop.GetSrcObject<FbxFileTexture>(0);
            if (!tex) return;

            // 경로 계산
            std::filesystem::path baseDir = std::filesystem::path(basePath).parent_path();
            std::filesystem::path texPath = tex->GetFileName();
            std::filesystem::path relPath = texPath.filename();
            std::filesystem::path fullPath = baseDir / relPath;

            // 캐시 확인
            auto existTex = rs->GetByPath<Texture>(fullPath.string());
            if (existTex)
            {
                texId = existTex->GetId();
                texSlot = existTex->GetSlot();
                textureIds.push_back(texId);
                OutputDebugStringA(("[TextureLoader-FBX] Reused cached texture: " + fullPath.string() + "\n").c_str());
                return;
            }

            // 새 텍스처 생성
            auto newTex = std::make_shared<Texture>();
            if (!newTex->LoadFromFile(fullPath.string(), ctx))
            {
                OutputDebugStringA(("[TextureLoader-FBX] Failed to load texture: " + fullPath.string() + "\n").c_str());
                return;
            }

            newTex->SetPath(fullPath.string());
            newTex->SetAlias(fullPath.stem().string());
            rs->RegisterResource(newTex);  

            texId = newTex->GetId();
            texSlot = newTex->GetSlot();
            textureIds.push_back(texId);

            OutputDebugStringA(("[TextureLoader-FBX] Loaded new texture: " + fullPath.string() + "\n").c_str());
        };

    // FBX 기본 속성 이름 (DX12 PBR 기준)
    LoadTex(FbxSurfaceMaterial::sDiffuse, mat->diffuseTexId, mat->diffuseTexSlot);
    LoadTex(FbxSurfaceMaterial::sNormalMap, mat->normalTexId, mat->normalTexSlot);
    LoadTex(FbxSurfaceMaterial::sSpecular, mat->roughnessTexId, mat->roughnessTexSlot);
    LoadTex(FbxSurfaceMaterial::sReflection, mat->metallicTexId, mat->metallicTexSlot);

    return textureIds;
}
