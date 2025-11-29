#pragma once
#include "Game_Resource.h"
#include "Skeleton.h"

class AvatarMask : public Game_Resource
{
public:
    AvatarMask();
    virtual ~AvatarMask() = default;

    virtual bool LoadFromFile(std::string path, const RendererContext& ctx) override;
    virtual bool SaveToFile(const std::string& path) const override;

    void ExpandHierarchy(const Skeleton* skeleton, const std::vector<std::string>& boneToKeyMap);

    float GetWeight(const std::string& abstractKey) const;
    void SetWeight(const std::string& abstractKey, float weight);
    void Clear();

    const std::unordered_map<std::string, float>& GetMetaMap() const { return mWeights; }

private:
    std::unordered_map<std::string, float> mWeights;
};