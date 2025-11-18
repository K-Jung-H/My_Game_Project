#pragma once
#include "Game_Resource.h"

struct Bone
{
    std::string name;
    int parentIndex;
    XMFLOAT4X4 inverseBind;
};


class Skeleton : public Game_Resource
{
    friend class ModelLoader_FBX;
	friend class ModelLoader_Assimp;
	friend class AnimationControllerComponent;
    friend class SkinnedMesh;
    friend class DX12_Renderer;

private:
    std::vector<Bone> BoneList;
    std::unordered_set<std::string> BoneNames;

    std::vector<XMFLOAT4X4> mBindGlobal;
    std::vector<XMFLOAT4X4> mBindLocal;

    std::unordered_map<std::string, int> NameToIndex;

public:
    virtual bool LoadFromFile(std::string path, const RendererContext& ctx) override;
    virtual bool SaveToFile(const std::string& path) const;

    void SortBoneList();
    UINT GetBoneCount() const { return BoneList.size(); }
    const std::vector<Bone>& GetBoneList() const { return BoneList; }
    const std::unordered_map<std::string, int>& GetNameToIndexMap() const { return NameToIndex; }

    const std::vector<XMFLOAT4X4>& GetBindGlobalList() const { return mBindGlobal; }
    const std::vector<XMFLOAT4X4>& GetBindLocalList() const { return mBindLocal; }

    const XMFLOAT4X4& GetBindGlobal(int index) const { return mBindGlobal[index]; }
    const XMFLOAT4X4& GetBindLocal(int index) const { return mBindLocal[index]; }

    void BuildNameToIndex();
    void BuildBindPoseTransforms();
};