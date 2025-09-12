#pragma once
#include "pch.h"
#include "Game_Resource.h"
#include "ResourceManager.h"
#include <assimp/Importer.hpp>

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

    LoadResult Load(ResourceManager& manager, const std::string& path, std::string_view alias);

private:
    ResourceRegistry() = default;
    ~ResourceRegistry() = default;

    UINT LoadMaterialTexture(ResourceManager& manager, aiMaterial* material, aiTextureType type, UINT& nextId, const std::string& basePath, const std::string& suffix, std::vector<UINT>& outTextureIds);

    UINT mNextResourceID = 1;
};

