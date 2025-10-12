#pragma once
#include "ResourceSystem.h"

class ModelLoader_Assimp
{
public:
    explicit ModelLoader_Assimp();
    bool Load(const std::string& path, std::string_view alias, LoadResult& result);

private:
    std::shared_ptr<Model::Node> ProcessNode(aiNode* node, const aiScene* scene, const std::vector<std::shared_ptr<Mesh>>& loadedMeshes);
    Skeleton BuildSkeleton(const aiScene* scene);
};
