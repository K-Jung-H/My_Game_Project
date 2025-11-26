#pragma once
#include "ResourceSystem.h"

class ModelLoader_Assimp
{
public:
    explicit ModelLoader_Assimp();
    bool Load(const std::string& path, std::string_view alias, LoadResult& result);

private:
    // 노드 계층 구조 재귀 처리
    std::shared_ptr<Model::Node> ProcessNode(
        const RendererContext& ctx,
        aiNode* node,
        const aiScene* scene,
        const std::string& path,
        const std::vector<UINT>& materialIDs,
        std::vector<std::shared_ptr<Mesh>>& outLoadedMeshes);

    // 메쉬 생성 (캐싱 지원)
    std::shared_ptr<Mesh> CreateMeshFromNode(
        unsigned int meshIndex,
        const aiScene* scene,
        const std::string& path,
        const std::vector<UINT>& materialIDs);

    // 스켈레톤 구축 (mNames + mBones 구조)
    std::shared_ptr<Skeleton> BuildSkeleton(const aiScene* scene);

    // 애니메이션 변환
    std::shared_ptr<AnimationClip> ProcessAnimation(
        const aiAnimation* anim,
        std::shared_ptr<Model_Avatar> avatar,
        std::shared_ptr<Skeleton> skeleton);

private:
    // [중요] 메쉬 중복 로드 방지용 캐시 (aiMesh Index -> Mesh Resource)
    std::unordered_map<unsigned int, std::shared_ptr<Mesh>> m_meshMap;
};
