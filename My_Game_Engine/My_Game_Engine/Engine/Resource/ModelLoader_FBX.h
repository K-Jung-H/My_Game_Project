#pragma once
#include "ResourceSystem.h"

class ModelLoader_FBX
{
public:
    explicit ModelLoader_FBX();
    bool Load(const std::string& path, std::string_view alias, LoadResult& result);

private:
    std::shared_ptr<Model::Node> ProcessNode(const RendererContext& ctx, FbxNode* fbxNode, std::unordered_map<FbxSurfaceMaterial*, UINT>& matMap,
        const std::string& path, std::vector<std::shared_ptr<Mesh>>& loadedMeshes);

    std::shared_ptr<Mesh> CreateMeshFromNode(FbxNode* fbxNode,
        std::unordered_map<FbxSurfaceMaterial*, UINT>& matMap, const std::string& path);

    Skeleton BuildSkeleton(FbxScene* fbxScene);

    std::unordered_map<FbxMesh*, std::shared_ptr<Mesh>> m_meshMap;
};
