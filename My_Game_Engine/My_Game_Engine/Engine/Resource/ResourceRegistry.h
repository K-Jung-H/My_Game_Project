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
    bool LoadWithAssimp(ResourceManager& manager, const std::string& path, std::string_view aalias, const RendererContext& ctx, LoadResult& result);
    bool LoadWithFbxSdk(ResourceManager& manager, const std::string& path, std::string_view aalias, const RendererContext& ctx, LoadResult& result);

private:
    ResourceRegistry() = default;
    ~ResourceRegistry() = default;

    //-------------------------------Assimp-----------------------------------

    std::vector<UINT> LoadMaterialTextures_Assimp(ResourceManager& manager, aiMaterial* material, const std::string& basePath, std::shared_ptr<Material>& mat, const RendererContext& ctx);
    std::shared_ptr<Model::Node> ProcessNode(aiNode* ainode, const aiScene* scene, const std::vector<std::shared_ptr<Mesh>>& loadedMeshes);
    Skeleton BuildSkeleton_Assimp(const aiScene* scene);

    //-------------------------------FBX SDK-----------------------------------

    std::vector<UINT> LoadMaterialTexturesFbx(ResourceManager& manager, FbxSurfaceMaterial* fbxMat, 
        const std::string& basePath, std::shared_ptr<Material>& mat, const RendererContext& ctx);

    std::shared_ptr<Model::Node> ProcessFbxNode(FbxNode* fbxNode, ResourceManager& manager, std::vector<UINT>& matMap, 
        const std::string& path, const RendererContext& ctx, std::vector<std::shared_ptr<Mesh>>& loadedMeshes);

    std::shared_ptr<Mesh> CreateMeshFromFbxNode(FbxNode* fbxNode, ResourceManager& manager,
        std::vector<UINT>& matMap, const std::string& path, const RendererContext& ctx);

    Skeleton BuildSkeletonFbx(FbxScene* fbxScene);


    UINT mNextResourceID = 1;
};

