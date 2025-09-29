#pragma once
#include "Game_Resource.h"
#include "ResourceManager.h"
#include "Model.h"

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

private:
    ResourceRegistry() = default;
    ~ResourceRegistry() = default;

    std::vector<UINT> LoadMaterialTextures(ResourceManager& manager, aiMaterial* material, const std::string& basePath, std::shared_ptr<Material>& mat, const RendererContext& ctx);
    std::shared_ptr<Model::Node> ProcessNode(aiNode* ainode, const aiScene* scene, const std::vector<std::shared_ptr<Mesh>>& loadedMeshes);
    Skeleton BuildSkeleton(const aiScene* scene);

    UINT mNextResourceID = 1;
};

