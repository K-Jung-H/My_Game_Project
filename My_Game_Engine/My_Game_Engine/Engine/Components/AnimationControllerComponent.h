#pragma once
#include "Core/Component.h"
#include "Resource/Skeleton.h"
#include "Resource/Model_Avatar.h"
#include "Resource/AnimationClip.h"

struct BoneMask
{
    std::string name;
    std::vector<float> weights;

    void Initialize(size_t boneCount, float initialWeight = 1.0f)
    {
        weights.assign(boneCount, initialWeight);
    }

    void SetWeight(int boneIndex, float weight)
    {
        if (boneIndex >= 0 && boneIndex < (int)weights.size())
            weights[boneIndex] = weight;
    }

    float GetWeight(int boneIndex) const
    {
        if (boneIndex >= 0 && boneIndex < (int)weights.size())
            return weights[boneIndex];
        return 0.0f;
    }
};

enum class PlaybackMode
{
    Loop,
    Once,
    PingPong
};

struct AnimationState
{
    std::shared_ptr<AnimationClip> clip;
    PlaybackMode mode = PlaybackMode::Loop;
    float currentTime = 0.0f;
    float speed = 1.0f;
    float weight = 1.0f;
    bool  isValid = false;
    bool isReverse = false;

    std::shared_ptr<BoneMask> mask = nullptr;

    void Reset(std::shared_ptr<AnimationClip> newClip, float newSpeed, PlaybackMode newMode);
    void Update(float deltaTime);
};

struct BoneMatrixData
{
    XMFLOAT4X4 transform;
};

class AnimationControllerComponent : public Component
{
public:
    static constexpr Component_Type Type = Component_Type::AnimationController;
    Component_Type GetType() const override { return Type; }

public:
    AnimationControllerComponent();
    virtual ~AnimationControllerComponent() = default;

    void SetPlaybackMode(PlaybackMode mode);
    PlaybackMode GetPlaybackMode() const;

    void SetSkeleton(std::shared_ptr<Skeleton> skeleton);
    void SetModelAvatar(std::shared_ptr<Model_Avatar> model_avatar);
    void UpdateBoneMappingCache();

    std::shared_ptr<Skeleton> GetSkeleton() { return mModelSkeleton; }
    std::shared_ptr<Model_Avatar> GetModelAvatar() { return mModelAvatar; }

    std::shared_ptr<AnimationClip> GetCurrentClip() const { return mCurrentState.clip; }

    UINT GetBoneMatrixSRV() const { return mBoneMatrixSRVSlot; }

    void Play(std::shared_ptr<AnimationClip> clip, float blendTime, PlaybackMode mode, float speed);

    void SetSpeed(float speed) { mCurrentState.speed = speed; }
    float GetSpeed() const { return mCurrentState.speed; }

    bool IsReady() const;
    void Update(float deltaTime);

//    std::shared_ptr<BoneMask> CreateMask(const std::string& name);

private:
    void CreateBoneMatrixBuffer();

    void EvaluateAnimation();

private:
    std::shared_ptr<Model_Avatar> mModelAvatar;
    std::shared_ptr<Skeleton> mModelSkeleton;

    AnimationState mCurrentState;
    AnimationState mPrevState; 

    bool  mIsTransitioning = false;
    float mTransitionTime = 0.0f;
    float mTransitionDuration = 0.0f;

    std::vector<BoneMatrixData> mCpuBoneMatrices;
    ComPtr<ID3D12Resource> mBoneMatrixBuffer;
    BoneMatrixData* mMappedBoneBuffer = nullptr;
    UINT mBoneMatrixSRVSlot = UINT_MAX;

    std::vector<std::string> mCachedBoneToKey;
};