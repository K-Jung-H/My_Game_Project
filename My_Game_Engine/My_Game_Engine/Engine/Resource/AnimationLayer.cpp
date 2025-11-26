#include "AnimationLayer.h"


AnimationLayer::AnimationLayer()
{
}

void AnimationLayer::SetNormalizedTime(float ratio)
{
    if (mCurrentState.isValid && mCurrentState.clip)
    {
        float duration = mCurrentState.clip->GetDuration();
        mCurrentState.currentTime = duration * std::clamp(ratio, 0.0f, 1.0f);
    }
}

float AnimationLayer::GetNormalizedTime() const
{
    if (mCurrentState.isValid && mCurrentState.clip)
    {
        float duration = mCurrentState.clip->GetDuration();
        if (duration > 0.0f)
            return mCurrentState.currentTime / duration;
    }
    return 0.0f;
}

float AnimationLayer::GetCurrentDuration() const
{
    if (mCurrentState.isValid && mCurrentState.clip)
        return mCurrentState.clip->GetDuration();
    return 0.0f;
}

void AnimationLayer::Play(std::shared_ptr<AnimationClip> clip, float blendTime, PlaybackMode mode, float speed)
{
    if (!clip) return;
    if (mCurrentState.clip == clip && !mIsTransitioning) return;

    if (blendTime > 0.0f && mCurrentState.isValid)
    {
        mPrevState = mCurrentState;
        mIsTransitioning = true;
        mTransitionDuration = blendTime;
        mTransitionTime = 0.0f;
    }
    else
    {
        mIsTransitioning = false;
        mPrevState.isValid = false;
    }

    mCurrentState.Reset(clip, speed, mode);
}

void AnimationLayer::Update(float deltaTime)
{
    mCurrentState.Update(deltaTime);

    if (mIsTransitioning)
    {
        mPrevState.Update(deltaTime);
        mTransitionTime += deltaTime;

        float t = (mTransitionDuration > 0.0f) ? std::clamp(mTransitionTime / mTransitionDuration, 0.0f, 1.0f) : 1.0f;

        mPrevState.weight = 1.0f - t;
        mCurrentState.weight = t;

        if (t >= 1.0f)
        {
            mIsTransitioning = false;
            mPrevState.isValid = false;
        }
    }
    else
    {
        mCurrentState.weight = 1.0f;
    }
}

bool AnimationLayer::GetSample(const AnimationState& state, const std::string& key,
    XMVECTOR& s, XMVECTOR& r, XMVECTOR& t)
{
    if (!state.isValid || !state.clip) return false;

    const AnimationTrack* track = state.clip->GetTrack(key);
    if (!track) return false;

    XMFLOAT3 vs = track->SampleScale(state.currentTime);
    XMFLOAT4 vr = track->SampleRotation(state.currentTime);
    XMFLOAT3 vt = track->SamplePosition(state.currentTime);

    s = XMLoadFloat3(&vs);
    r = XMLoadFloat4(&vr);
    t = XMLoadFloat3(&vt);

    return true;
}

bool AnimationLayer::EvaluateAndBlend(
    const std::string& abstractKey,
    float maskWeight,
    XMVECTOR& inOutS,
    XMVECTOR& inOutR,
    XMVECTOR& inOutT)
{
    if (!mCurrentState.isValid || mLayerWeight <= 0.0f || maskWeight <= 0.0f)
        return false;

    float finalAlpha = mLayerWeight * maskWeight;
    if (finalAlpha <= 0.001f) return false;

    XMVECTOR s_curr, r_curr, t_curr;
    if (!GetSample(mCurrentState, abstractKey, s_curr, r_curr, t_curr))
        return false;

    if (mIsTransitioning && mPrevState.isValid)
    {
        XMVECTOR s_prev, r_prev, t_prev;
        if (GetSample(mPrevState, abstractKey, s_prev, r_prev, t_prev))
        {
            float crossFadeAlpha = mCurrentState.weight;
            s_curr = XMVectorLerp(s_prev, s_curr, crossFadeAlpha);
            t_curr = XMVectorLerp(t_prev, t_curr, crossFadeAlpha);
            r_curr = XMQuaternionSlerp(r_prev, r_curr, crossFadeAlpha);
        }
    }

    if (mBlendMode == LayerBlendMode::Override)
    {
        inOutS = XMVectorLerp(inOutS, s_curr, finalAlpha);
        inOutT = XMVectorLerp(inOutT, t_curr, finalAlpha);
        inOutR = XMQuaternionSlerp(inOutR, r_curr, finalAlpha);
    }
    else if (mBlendMode == LayerBlendMode::Additive)
    {
        // 추후 확장 시 구현
    }

    return true;
}