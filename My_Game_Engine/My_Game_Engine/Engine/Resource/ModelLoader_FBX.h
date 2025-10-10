#pragma once
#include "ResourceManager.h"
#include "ResourceRegistry.h"
#include "Model.h"

class ModelLoader_FBX
{
public:
    explicit ModelLoader_FBX();
    bool Load(ResourceManager& manager, const std::string& path, std::string_view alias, const RendererContext& ctx, LoadResult& result);

private:

    std::shared_ptr<Model::Node> ProcessNode(FbxNode* fbxNode, ResourceManager& manager, std::unordered_map<FbxSurfaceMaterial*, UINT>& matMap,
        const std::string& path, const RendererContext& ctx, std::vector<std::shared_ptr<Mesh>>& loadedMeshes);

    std::shared_ptr<Mesh> CreateMeshFromNode(FbxNode* fbxNode, ResourceManager& manager,
        std::unordered_map<FbxSurfaceMaterial*, UINT>& matMap, const std::string& path, const RendererContext& ctx);

    Skeleton BuildSkeleton(FbxScene* fbxScene);
};
