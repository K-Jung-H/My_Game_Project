#include "AnimationControllerComponent.h"
#include "Core/Object.h"
#include "Resource/Model.h"
#include "GameEngine.h"
#include "DX_Graphics/ResourceUtils.h"


void AnimationState::Reset(std::shared_ptr<AnimationClip> newClip, float newSpeed, PlaybackMode newMode)
{
    clip = newClip;
    speed = newSpeed;
    mode = newMode;

    currentTime = 0.0f;
    weight = 1.0f;
    isValid = (clip != nullptr);
    isReverse = false;
}

void AnimationState::Update(float deltaTime)
{
    if (!isValid || !clip) return;

    float duration = clip->GetDuration();
    if (duration <= 0.0f) return;

    float actualSpeed = speed;
    if (mode == PlaybackMode::PingPong && isReverse)
    {
        actualSpeed = -speed;
    }

    currentTime += deltaTime * actualSpeed;

    if (mode == PlaybackMode::Loop)
    {
        if (currentTime >= duration)
            currentTime = fmod(currentTime, duration);
        else if (currentTime < 0.0f)
            currentTime = duration + fmod(currentTime, duration);
    }
    else if (mode == PlaybackMode::Once)
    {
        if (currentTime >= duration) currentTime = duration;
        else if (currentTime <= 0.0f) currentTime = 0.0f;
    }
    else if (mode == PlaybackMode::PingPong)
    {
        if (currentTime >= duration)
        {
            currentTime = duration;
            isReverse = true;
        }
        else if (currentTime <= 0.0f)
        {
            currentTime = 0.0f;
            isReverse = false;
        }
    }
}



AnimationControllerComponent::AnimationControllerComponent()
    : Component()
{
}

void AnimationControllerComponent::CreateBoneMatrixBuffer()
{
    if (!mModelSkeleton || mBoneMatrixBuffer) return;

    const size_t boneCount = mModelSkeleton->GetBoneCount();
    if (boneCount == 0) return;

    mCpuBoneMatrices.resize(boneCount);

    RendererContext rc = GameEngine::Get().Get_UploadContext();
    DescriptorManager* heap = rc.resourceHeap;

    const UINT bufferSize = sizeof(BoneMatrixData) * (UINT)boneCount;

    mBoneMatrixBuffer = ResourceUtils::CreateBufferResourceEmpty(
        rc, bufferSize,
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_RESOURCE_FLAG_NONE,
        D3D12_RESOURCE_STATE_GENERIC_READ
    );

    HRESULT hr = mBoneMatrixBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedBoneBuffer));
    if (FAILED(hr))
    {
        OutputDebugStringA("[AnimationControllerComponent] Error: Bone Matrix 버퍼 맵핑 실패.\n");
        return;
    }

    mBoneMatrixSRVSlot = heap->Allocate(HeapRegion::SRV_Static);
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.NumElements = (UINT)boneCount;
    srvDesc.Buffer.StructureByteStride = sizeof(BoneMatrixData);
    rc.device->CreateShaderResourceView(mBoneMatrixBuffer.Get(), &srvDesc, heap->GetCpuHandle(mBoneMatrixSRVSlot));
}

void AnimationControllerComponent::SetPlaybackMode(PlaybackMode mode)
{
    mCurrentState.mode = mode;
}

PlaybackMode AnimationControllerComponent::GetPlaybackMode() const
{
    return mCurrentState.mode;
}

void AnimationControllerComponent::SetSkeleton(std::shared_ptr<Skeleton> skeleton)
{
    if (mModelSkeleton == skeleton) return;

    mModelSkeleton = skeleton;

    if (mModelSkeleton && !mBoneMatrixBuffer)
    {
        CreateBoneMatrixBuffer();
    }
    else if (!mModelSkeleton)
    {
        if (mMappedBoneBuffer)
        {
            mBoneMatrixBuffer->Unmap(0, nullptr);
            mMappedBoneBuffer = nullptr;
        }

        if (mBoneMatrixSRVSlot != UINT_MAX)
        {
            const RendererContext ctx = GameEngine::Get().Get_UploadContext();

            mBoneMatrixBuffer.Reset();
            ctx.resourceHeap->FreeDeferred(HeapRegion::SRV_Static, mBoneMatrixSRVSlot);

            mBoneMatrixSRVSlot = UINT_MAX;
        }
    }

    UpdateBoneMappingCache();
}

void AnimationControllerComponent::SetModelAvatar(std::shared_ptr<Model_Avatar> model_avatar)
{
    mModelAvatar = model_avatar;

    UpdateBoneMappingCache();
}

void AnimationControllerComponent::UpdateBoneMappingCache()
{
    if (!mModelSkeleton || !mModelAvatar) return;

    size_t boneCount = mModelSkeleton->GetBoneCount();
    mCachedBoneToKey.clear();
    mCachedBoneToKey.resize(boneCount);

    for (size_t i = 0; i < boneCount; ++i)
    {
        std::string boneName = mModelSkeleton->GetBoneName((int)i);
        mCachedBoneToKey[i] = mModelAvatar->GetMappedKeyByBoneName(boneName);
    }
}

bool AnimationControllerComponent::IsReady() const
{
    return mModelSkeleton != nullptr && mModelAvatar != nullptr && mBoneMatrixSRVSlot != UINT_MAX;
}

void AnimationControllerComponent::Play(std::shared_ptr<AnimationClip> clip, float blendTime, PlaybackMode mode, float speed)
{
    if (!clip || !IsReady()) return;

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

void AnimationControllerComponent::Update(float deltaTime)
{
    if (!IsReady()) return;

    mCurrentState.Update(deltaTime);

    if (mIsTransitioning)
    {
        mPrevState.Update(deltaTime);

        mTransitionTime += deltaTime;

        float t = 0.0f;
        if (mTransitionDuration > 0.0f)
        {
            t = std::clamp(mTransitionTime / mTransitionDuration, 0.0f, 1.0f);
        }
        else
        {
            t = 1.0f;
        }

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

    EvaluateAnimation();

    if (mMappedBoneBuffer)
    {
        memcpy(mMappedBoneBuffer, mCpuBoneMatrices.data(), sizeof(BoneMatrixData) * mCpuBoneMatrices.size());
    }
}
void AnimationControllerComponent::EvaluateAnimation()
{
    using namespace DirectX;

    if (!mModelSkeleton || !mModelAvatar) return;

    const auto& bones = mModelSkeleton->GetBones();
    const size_t boneCount = bones.size();

    if (mCpuBoneMatrices.size() != boneCount)
    {
        mCpuBoneMatrices.resize(boneCount);
    }

    std::vector<XMMATRIX> localTransforms(boneCount);

    for (size_t i = 0; i < boneCount; ++i)
    {
        const std::string& abstractKey = mCachedBoneToKey[i];

        const BoneInfo& boneInfo = bones[i];
        XMMATRIX bindLocal = XMLoadFloat4x4(&boneInfo.bindLocal);

        XMVECTOR S_final, R_final, T_final;
        XMMatrixDecompose(&S_final, &R_final, &T_final, bindLocal);

        if (!abstractKey.empty())
        {
            if (mCurrentState.isValid)
            {
                const AnimationTrack* track = mCurrentState.clip->GetTrack(abstractKey);
                if (track)
                {
                    XMFLOAT3 s = track->SampleScale(mCurrentState.currentTime);
                    XMFLOAT4 r = track->SampleRotation(mCurrentState.currentTime);
                    XMFLOAT3 t = track->SamplePosition(mCurrentState.currentTime);

                    S_final = XMLoadFloat3(&s);
                    R_final = XMLoadFloat4(&r);
                    T_final = XMLoadFloat3(&t);
                }
            }

            if (mIsTransitioning && mPrevState.isValid)
            {
                const AnimationTrack* prevTrack = mPrevState.clip->GetTrack(abstractKey);
                if (prevTrack)
                {
                    XMFLOAT3 s_prev = prevTrack->SampleScale(mPrevState.currentTime);
                    XMFLOAT4 r_prev = prevTrack->SampleRotation(mPrevState.currentTime);
                    XMFLOAT3 t_prev = prevTrack->SamplePosition(mPrevState.currentTime);

                    XMVECTOR S_p = XMLoadFloat3(&s_prev);
                    XMVECTOR R_p = XMLoadFloat4(&r_prev);
                    XMVECTOR T_p = XMLoadFloat3(&t_prev);

                    float blendAlpha = mCurrentState.weight;
                    if (mCurrentState.mask)
                    {
                        blendAlpha *= mCurrentState.mask->GetWeight((int)i);
                    }

                    S_final = XMVectorLerp(S_p, S_final, blendAlpha);
                    T_final = XMVectorLerp(T_p, T_final, blendAlpha);
                    R_final = XMQuaternionSlerp(R_p, R_final, blendAlpha);
                }
            }
        }

        localTransforms[i] =
            XMMatrixScalingFromVector(S_final) * XMMatrixRotationQuaternion(R_final) * XMMatrixTranslationFromVector(T_final);
    }

    std::vector<XMMATRIX> globalTransforms(boneCount);

    for (size_t i = 0; i < boneCount; ++i)
    {
        int parent = bones[i].parentIndex;
        if (parent < 0)
            globalTransforms[i] = localTransforms[i];
        else
            globalTransforms[i] = localTransforms[i] * globalTransforms[parent];
    }

    for (size_t i = 0; i < boneCount; ++i)
    {
        XMMATRIX invBind = XMLoadFloat4x4(&bones[i].inverseBind);
        XMMATRIX finalMat = XMMatrixTranspose(invBind * globalTransforms[i]);
        XMStoreFloat4x4(&mCpuBoneMatrices[i].transform, finalMat);
    }
}