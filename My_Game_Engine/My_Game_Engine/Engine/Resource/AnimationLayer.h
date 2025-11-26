#pragma once
#include "AnimationCommon.h"
#include "AvatarMask.h"


class AnimationLayer
{
public:
    AnimationLayer();
    ~AnimationLayer() = default;

    void Play(std::shared_ptr<AnimationClip> clip, float blendTime = 0.2f, PlaybackMode mode = PlaybackMode::Loop, float speed = 1.0f);
    void Update(float deltaTime);

    bool EvaluateAndBlend(
        const std::string& abstractKey,
        float maskWeight,
        DirectX::XMVECTOR& inOutS,
        DirectX::XMVECTOR& inOutR,
        DirectX::XMVECTOR& inOutT
    );

    void SetWeight(float w) { mLayerWeight = w; }
    float GetWeight() const { return mLayerWeight; }

    void SetMask(std::shared_ptr<AvatarMask> mask) { mMask = mask; }
    std::shared_ptr<AvatarMask> GetMask() const { return mMask; }

    void SetNormalizedTime(float ratio);
    float GetNormalizedTime() const;

    void SetBlendMode(LayerBlendMode mode) { mBlendMode = mode; }
    LayerBlendMode GetBlendMode() const { return mBlendMode; }

    void SetSpeed(float speed) { mCurrentState.speed = speed; }
    float GetSpeed() const { return mCurrentState.speed; }

    void SetPlaybackMode(PlaybackMode mode) { mCurrentState.mode = mode; }
    PlaybackMode GetPlaybackMode() const { return mCurrentState.mode; }


    bool IsTransitioning() const { return mIsTransitioning; }
    float GetTransitionProgress() const {
        if (mTransitionDuration <= 0.0f) return 1.0f;
        return std::clamp(mTransitionTime / mTransitionDuration, 0.0f, 1.0f);
    }
    float GetTransitionTime() const { return mTransitionTime; }
    float GetTransitionDuration() const { return mTransitionDuration; }
    float GetCurrentDuration() const;
    std::shared_ptr<AnimationClip> GetCurrentClip() const { return mCurrentState.clip; }


private:
    bool GetSample(const AnimationState& state, const std::string& key,
        DirectX::XMVECTOR& s, DirectX::XMVECTOR& r, DirectX::XMVECTOR& t);

private:
    std::string mName;
    float mLayerWeight = 1.0f;
    LayerBlendMode mBlendMode = LayerBlendMode::Override;
    std::shared_ptr<AvatarMask> mMask = nullptr;

    AnimationState mCurrentState;
    AnimationState mPrevState;

    bool mIsTransitioning = false;
    float mTransitionTime = 0.0f;
    float mTransitionDuration = 0.0f;
};