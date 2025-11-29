#pragma once
#include "Game_Resource.h"


struct BoneInfo
{
    int parentIndex;
    XMFLOAT4X4 bindLocal;
    XMFLOAT4X4 inverseBind;

    XMFLOAT3 bindScale;
    XMFLOAT4 bindRotation;
    XMFLOAT3 bindTranslation;
};

class Skeleton : public Game_Resource
{
    friend class ModelLoader_FBX;
    friend class ModelLoader_Assimp;
    friend class AnimationControllerComponent;
    friend class SkinnedMesh;
    friend class DX12_Renderer;

public:
    Skeleton();
    virtual ~Skeleton() = default;

    virtual bool LoadFromFile(std::string path, const RendererContext& ctx) override;
    virtual bool SaveToFile(const std::string& path) const;

    void SortBoneList();

    const std::vector<BoneInfo>& GetBones() const;
    const BoneInfo& GetBone(int index) const;
    const BoneInfo& GetBone(const std::string& name) const;
    UINT GetBoneCount() const;

    const std::string& GetBoneName(int index) const;
    int GetBoneIndex(const std::string& name) const;
    int GetRootBoneIndex() const;
private:
    void BuildNameToIndex();

private:
    std::vector<std::string> mNames;
    std::unordered_map<std::string, int> mNameToIndex;

    std::vector<BoneInfo> mBones;
    int mCachedRootIndex = -1;
};