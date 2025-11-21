#pragma once
#include "Core/Component.h"
#include "Resource/Skeleton.h"
#include "Resource/Model_Avatar.h"
#include "Resource/AnimationClip.h"

struct BoneMatrixData
{
    XMFLOAT4X4 transform;
};

enum class PlaybackMode
{
    Loop,
    Once,
    PingPong
};

class AnimationControllerComponent : public Component
{
public:
    static constexpr Component_Type Type = Component_Type::AnimationController;
    Component_Type GetType() const override { return Type; }

public:
    AnimationControllerComponent();
    virtual ~AnimationControllerComponent() = default;

    void SetSkeleton(std::shared_ptr<Skeleton> skeleton);
    void SetModelAvatar(std::shared_ptr<Model_Avatar> model_avatar);

    std::shared_ptr<Skeleton> GetSkeleton() { return mModelSkeleton; }
	std::shared_ptr<Model_Avatar> GetModelAvatar() { return mModelAvatar; }
    std::shared_ptr<AnimationClip> GetCurrentClip() const { return mCurrentClip; }

    UINT GetBoneMatrixSRV() const { return mBoneMatrixSRVSlot; }

    void SetSpeed(float speed) { mSpeed = speed; }
    float GetSpeed() const { return mSpeed; }

    void SetPlaybackMode(PlaybackMode mode) { mPlaybackMode = mode; }
    PlaybackMode GetPlaybackMode() const { return mPlaybackMode; }

    bool IsReady() const;
    void Update(float deltaTime);
    void Play(std::shared_ptr<AnimationClip> clip, PlaybackMode mode = PlaybackMode::Loop);


private:
    void CreateBoneMatrixBuffer();
    void EvaluateAnimation(float time);

private:
    std::shared_ptr<Model_Avatar> mModelAvatar;
    std::shared_ptr<Skeleton> mModelSkeleton;
    std::unordered_map<std::string, int> mMappedModelBoneIndex;

    std::shared_ptr<AnimationClip> mCurrentClip;
    std::shared_ptr<Model_Avatar> mClipAvatar;
    std::shared_ptr<Skeleton> mClipSkeleton;
    std::unordered_map<std::string, int> mMappedClipBoneIndex;

    PlaybackMode mPlaybackMode = PlaybackMode::Loop;
    bool mIsReverse = false;
    float mCurrentTime = 0.0f;
    float mSpeed = 1.0f;

    std::vector<BoneMatrixData> mCpuBoneMatrices;

    ComPtr<ID3D12Resource> mBoneMatrixBuffer;
    BoneMatrixData* mMappedBoneBuffer = nullptr;
    UINT mBoneMatrixSRVSlot = UINT_MAX;

};