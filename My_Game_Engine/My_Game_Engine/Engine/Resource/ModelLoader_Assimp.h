#pragma once
#include "ResourceManager.h"
#include "ResourceRegistry.h"
#include "Model.h"

class ModelLoader_Assimp
{
public:
    explicit ModelLoader_Assimp();
    bool Load(ResourceManager& manager, const std::string& path, std::string_view alias, const RendererContext& ctx, LoadResult& result);

private:
    std::shared_ptr<Model::Node> ProcessNode(aiNode* node, const aiScene* scene, const std::vector<std::shared_ptr<Mesh>>& loadedMeshes);
    Skeleton BuildSkeleton(const aiScene* scene);
};
