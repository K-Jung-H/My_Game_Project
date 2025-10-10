#pragma once
#include "ResourceManager.h"


struct LoadResult 
{
    std::vector<UINT> meshIds;
    std::vector<UINT> materialIds;
    std::vector<UINT> textureIds;
    UINT modelId = Engine::INVALID_ID;
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

    static UINT GenerateID() { return ++mNextResourceID; }
    static UINT PeekNextID() { return mNextResourceID; }

private:
    ResourceRegistry() = default;
    ~ResourceRegistry() = default;

    static inline UINT mNextResourceID = 1;
};

