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

    std::unordered_map<std::string, int> NameToIndex;

public:
    virtual bool LoadFromFile(std::string path, const RendererContext& ctx) { return false; }
    void SortBoneList();
    UINT GetBoneCount() const { return BoneList.size(); }

    void BuildNameToIndex()
    {
        NameToIndex.clear();
        for (size_t i = 0; i < BoneList.size(); i++)
            NameToIndex[BoneList[i].name] = static_cast<int>(i);
    }
};