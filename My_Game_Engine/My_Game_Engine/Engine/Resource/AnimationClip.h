#pragma once
#include "Game_Resource.h"
#include "AvatarSystem.h"

template<typename T>
struct TKey
{
    float time;
    T value;
};

using VectorKey = TKey<XMFLOAT3>;
using QuatKey = TKey<XMFLOAT4>;

class AnimationTrack
{
public:
    XMFLOAT3 SamplePosition(float time) const;
    XMFLOAT4 SampleRotation(float time) const;
    XMFLOAT3 SampleScale(float time) const;

    XMMATRIX Sample(float time) const;

public:
    std::vector<VectorKey> PositionKeys;
    std::vector<QuatKey>   RotationKeys;
    std::vector<VectorKey> ScaleKeys;
};

class AnimationClip : public Game_Resource
{
public:
    AnimationClip();
    virtual ~AnimationClip() = default;

    virtual bool LoadFromFile(std::string path, const RendererContext& ctx) override;
    virtual bool SaveToFile(const std::string& path) const;

    DefinitionType GetDefinitionType() const { return mAvatarDefinitionType; }
    float GetDuration() const { return mDuration; }
    const AnimationTrack* GetTrack(const std::string& boneKey) const;

public:
    DefinitionType mAvatarDefinitionType = DefinitionType::None;
    float mDuration = 0.0f;
    float mTicksPerSecond = 0.0f;

    // <추상 뼈 이름, 해당 뼈의 트랙>
    std::map<std::string, AnimationTrack> mTracks;
};