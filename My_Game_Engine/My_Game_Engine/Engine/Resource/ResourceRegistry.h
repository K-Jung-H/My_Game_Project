#pragma once
#include "pch.h"
#include "Game_Resource.h"
#include "ResourceManager.h"


struct LoadResult 
{
    std::vector<UINT> meshIds;
    std::vector<UINT> materialIds;
    std::vector<UINT> textureIds;
};

class ResourceRegistry
{
public:
    static ResourceRegistry& Instance()
    {
        static ResourceRegistry instance;
        return instance;
    }

    ResourceRegistry(const ResourceRegistry&) = delete;
    ResourceRegistry& operator=(const ResourceRegistry&Temp) = delete;

    LoadResult Load(ResourceManager& manager, const std::string& path, std::string_view alias, const RendererContext& ctx);

private:
    ResourceRegistry() = default;
    ~ResourceRegistry() = default;

//    UINT LoadMaterialTexture(ResourceManager& manager, aiMaterial* material, aiTextureType type, UINT& nextId, const std::string& basePath, const std::string& suffix, std::vector<UINT>& outTextureIds, const RendererContext& ctx);
    std::vector<UINT> LoadMaterialTextures(ResourceManager& manager, aiMaterial* material, const std::string& basePath, std::shared_ptr<Material>& mat, const RendererContext& ctx);

    UINT mNextResourceID = 1;
};

