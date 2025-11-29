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
    mPrevFrameTime = mCurrentState.currentTime;

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

void AnimationLayer::UpdateMaskCache(size_t boneCount, const std::vector<std::string>& boneToKeyMap)
{
    if (mBoneCache.size() != boneCount)
    {
        mBoneCache.resize(boneCount);
    }

    for (size_t i = 0; i < boneCount; ++i)
    {
        if (mMask && !boneToKeyMap[i].empty())
        {
            mBoneCache[i].maskWeight = mMask->GetWeight(boneToKeyMap[i]);
        }
        else
        {
            mBoneCache[i].maskWeight = 1.0f;
        }
    }
}

void AnimationLayer::UpdateTrackCache(size_t boneCount, const std::vector<std::string>& boneToKeyMap)
{
    if (mBoneCache.size() != boneCount)
    {
        mBoneCache.resize(boneCount);
    }

    bool hasCurrent = (mCurrentState.isValid && mCurrentState.clip);
    bool hasPrev = (mIsTransitioning && mPrevState.isValid && mPrevState.clip);

    for (size_t i = 0; i < boneCount; ++i)
    {
        LayerBoneCache& cache = mBoneCache[i];

        cache.currentTrack = nullptr;
        cache.prevTrack = nullptr;

        const std::string& key = boneToKeyMap[i];
        if (key.empty()) continue;

        if (hasCurrent)
        {
            cache.currentTrack = mCurrentState.clip->GetTrack(key);
        }

        if (hasPrev)
        {
            cache.prevTrack = mPrevState.clip->GetTrack(key);
        }
    }
}

float AnimationLayer::GetCachedMaskWeight(int boneIndex) const
{
    if (boneIndex >= 0 && boneIndex < mBoneCache.size())
    {
        return mBoneCache[boneIndex].maskWeight;
    }
    return 1.0f;
}

bool AnimationLayer::GetSample(const AnimationState& state, const std::string& key, XMVECTOR& s, XMVECTOR& r, XMVECTOR& t)
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

std::pair<XMVECTOR, XMVECTOR> AnimationLayer::GetRootMotionDelta(float deltaTime)
{
    if (!mCurrentState.clip || !mEnableRootMotion)
    {
        return { XMVectorZero(), XMQuaternionIdentity() };
    }

    const AnimationTrack* rootTrack = mCurrentState.clip->GetRootTrack();
    if (!rootTrack)
    {
        return { XMVectorZero(), XMQuaternionIdentity() };
    }

    float prevTime = mPrevFrameTime;
    float currTime = mCurrentState.currentTime;

    XMVECTOR p1, r1, s1; 
    XMVECTOR p2, r2, s2; 

    rootTrack->Sample(prevTime, s1, r1, p1);
    rootTrack->Sample(currTime, s2, r2, p2);

    XMVECTOR deltaPos = XMVectorSubtract(p2, p1);
    XMVECTOR deltaRot = XMQuaternionMultiply(r2, XMQuaternionInverse(r1));

    if (mCurrentState.mode == PlaybackMode::Loop && currTime < prevTime)
    {
        float duration = mCurrentState.clip->GetDuration();
        XMVECTOR startP, startR, startS;
        XMVECTOR endP, endR, endS;

        rootTrack->Sample(0.0f, startS, startR, startP);
        rootTrack->Sample(duration, endS, endR, endP);

        XMVECTOR deltaTailP = XMVectorSubtract(endP, p1);
        XMVECTOR deltaTailR = XMQuaternionMultiply(endR, XMQuaternionInverse(r1));

        XMVECTOR deltaHeadP = XMVectorSubtract(p2, startP);
        XMVECTOR deltaHeadR = XMQuaternionMultiply(r2, XMQuaternionInverse(startR));

        deltaPos = XMVectorAdd(deltaTailP, deltaHeadP);
        deltaRot = XMQuaternionMultiply(deltaTailR, deltaHeadR);
    }

    return { deltaPos, deltaRot };
}

bool AnimationLayer::EvaluateAndBlend(
    int boneIndex,
    XMVECTOR& inOutS,
    XMVECTOR& inOutR,
    XMVECTOR& inOutT)
{
    if (!mCurrentState.isValid || mLayerWeight <= 0.0f)
        return false;

    if (boneIndex < 0 || boneIndex >= mBoneCache.size()) return false;
    const LayerBoneCache& cache = mBoneCache[boneIndex];

    if (cache.maskWeight <= 0.0f) return false;

    float finalAlpha = mLayerWeight * cache.maskWeight;
    if (finalAlpha <= 0.001f) return false;

    if (!cache.currentTrack) return false;

    XMVECTOR s_curr, r_curr, t_curr;
    cache.currentTrack->Sample(mCurrentState.currentTime, s_curr, r_curr, t_curr);

    if (mIsTransitioning && mPrevState.isValid && cache.prevTrack)
    {
        XMVECTOR s_prev, r_prev, t_prev;
        cache.prevTrack->Sample(mPrevState.currentTime, s_prev, r_prev, t_prev);

        float crossFadeAlpha = mCurrentState.weight;
        s_curr = XMVectorLerp(s_prev, s_curr, crossFadeAlpha);
        t_curr = XMVectorLerp(t_prev, t_curr, crossFadeAlpha);
        r_curr = XMQuaternionSlerp(r_prev, r_curr, crossFadeAlpha);
    }

    if (mBlendMode == LayerBlendMode::Override)
    {
        inOutS = XMVectorLerp(inOutS, s_curr, finalAlpha);
        inOutT = XMVectorLerp(inOutT, t_curr, finalAlpha);
        inOutR = XMQuaternionSlerp(inOutR, r_curr, finalAlpha);
    }

    return true;
}