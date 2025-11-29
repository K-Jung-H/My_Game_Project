#include "AnimationControllerComponent.h"
#include "Core/Object.h"
#include "Resource/Model.h"
#include "GameEngine.h"
#include "DX_Graphics/ResourceUtils.h"

AnimationControllerComponent::AnimationControllerComponent()
    : Component()
{
    mLayers.resize(1);
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
        OutputDebugStringA("[AnimationControllerComponent] Error: Bone Matrix Map Failed.\n");
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

bool AnimationControllerComponent::IsReady() const
{
    return mModelSkeleton != nullptr && mModelAvatar != nullptr && mBoneMatrixSRVSlot != UINT_MAX;
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

    for (auto& layer : mLayers)
    {
        auto mask = layer.GetMask();

        if (mask)
            mask->ExpandHierarchy(mModelSkeleton.get(), mCachedBoneToKey);

        layer.UpdateMaskCache(boneCount, mCachedBoneToKey);
        layer.UpdateTrackCache(boneCount, mCachedBoneToKey);
    }
}

void AnimationControllerComponent::SetLayerCount(int count)
{
    if (count < 1) count = 1;
    mLayers.resize(count);
}

void AnimationControllerComponent::SetLayerWeight(int layerIndex, float weight)
{
    if (layerIndex >= 0 && layerIndex < mLayers.size())
    {
        mLayers[layerIndex].SetWeight(weight);
    }
}

float AnimationControllerComponent::GetLayerWeight(int layerIndex) const
{
    if (layerIndex >= 0 && layerIndex < mLayers.size())
    {
        return mLayers[layerIndex].GetWeight();
    }
    return 0.0f;
}

void AnimationControllerComponent::SetLayerMask(int layerIndex, std::shared_ptr<AvatarMask> mask)
{
    if (layerIndex >= 0 && layerIndex < mLayers.size())
    {
        mLayers[layerIndex].SetMask(mask);

        if (mModelSkeleton)
        {
            if (mask)
                mask->ExpandHierarchy(mModelSkeleton.get(), mCachedBoneToKey);

            mLayers[layerIndex].UpdateMaskCache(mModelSkeleton->GetBoneCount(), mCachedBoneToKey);
        }
    }
}

std::shared_ptr<AvatarMask> AnimationControllerComponent::GetLayerMask(int layerIndex) const
{
    if (layerIndex >= 0 && layerIndex < mLayers.size())
    {
        return mLayers[layerIndex].GetMask();
    }
    return nullptr;
}

void AnimationControllerComponent::SetPlaybackMode(PlaybackMode mode, int layerIndex)
{
    if (layerIndex >= 0 && layerIndex < mLayers.size())
    {
        mLayers[layerIndex].SetPlaybackMode(mode);
    }
}

PlaybackMode AnimationControllerComponent::GetPlaybackMode(int layerIndex) const
{
    if (layerIndex >= 0 && layerIndex < mLayers.size())
    {
        return mLayers[layerIndex].GetPlaybackMode();
    }
    return PlaybackMode::Loop;
}

void AnimationControllerComponent::SetSpeed(float speed, int layerIndex)
{
    if (layerIndex >= 0 && layerIndex < mLayers.size())
    {
        mLayers[layerIndex].SetSpeed(speed);
    }
}

float AnimationControllerComponent::GetSpeed(int layerIndex) const
{
    if (layerIndex >= 0 && layerIndex < mLayers.size())
    {
        return mLayers[layerIndex].GetSpeed();
    }
    return 1.0f;
}

void AnimationControllerComponent::Play(int layerIndex, std::shared_ptr<AnimationClip> clip, float blendTime, PlaybackMode mode, float speed)
{
    if (!IsReady()) return;

    if (layerIndex < 0) return;

    if (layerIndex >= (int)mLayers.size())
        mLayers.resize(layerIndex + 1);

    mLayers[layerIndex].Play(clip, blendTime, mode, speed);

    if (mModelSkeleton)
        mLayers[layerIndex].UpdateTrackCache(mModelSkeleton->GetBoneCount(), mCachedBoneToKey);
}

bool AnimationControllerComponent::IsLayerTransitioning(int layerIndex) const
{
    if (layerIndex >= 0 && layerIndex < mLayers.size())
        return mLayers[layerIndex].IsTransitioning();
    return false;
}

float AnimationControllerComponent::GetLayerTransitionProgress(int layerIndex) const
{
    if (layerIndex >= 0 && layerIndex < mLayers.size())
        return mLayers[layerIndex].GetTransitionProgress();
    return 0.0f;
}

std::shared_ptr<AnimationClip> AnimationControllerComponent::GetCurrentClip(int layerIndex) const
{
    if (layerIndex >= 0 && layerIndex < mLayers.size())
    {
        return mLayers[layerIndex].GetCurrentClip();
    }
    return nullptr;
}

void AnimationControllerComponent::SetLayerNormalizedTime(int layerIndex, float ratio)
{
    if (layerIndex >= 0 && layerIndex < mLayers.size())
        mLayers[layerIndex].SetNormalizedTime(ratio);
}

float AnimationControllerComponent::GetLayerNormalizedTime(int layerIndex) const
{
    if (layerIndex >= 0 && layerIndex < mLayers.size())
        return mLayers[layerIndex].GetNormalizedTime();
    return 0.0f;
}

float AnimationControllerComponent::GetLayerDuration(int layerIndex) const
{
    if (layerIndex >= 0 && layerIndex < mLayers.size())
        return mLayers[layerIndex].GetCurrentDuration();
    return 0.0f;
}

void AnimationControllerComponent::Update(float deltaTime)
{
    if (!IsReady()) return;

    std::shared_ptr<TransformComponent> transform = mTransform.lock();
    if (!transform)
    {
        if (auto owner = GetOwner())
        {
            transform = owner->GetComponent<TransformComponent>();
            mTransform = transform;
        }
    }

    if (!mIsPaused)
    {
        for (auto& layer : mLayers)
        {
            layer.Update(deltaTime);
            layer.GetRootMotionDelta(deltaTime);
        }
    }

    EvaluateLayers();

    if (mMappedBoneBuffer)
    {
        memcpy(mMappedBoneBuffer, mCpuBoneMatrices.data(), sizeof(BoneMatrixData) * mCpuBoneMatrices.size());
    }
}

void AnimationControllerComponent::EvaluateLayers()
{
    if (!mModelSkeleton || !mModelAvatar) return;

    const auto& bones = mModelSkeleton->GetBones();
    const size_t boneCount = bones.size();

    if (mCpuBoneMatrices.size() != boneCount)
    {
        mCpuBoneMatrices.resize(boneCount);
    }

    std::vector<XMMATRIX> globalTransforms(boneCount);

    for (size_t i = 0; i < boneCount; ++i)
    {
        const BoneInfo& boneInfo = bones[i];
        int parentIdx = boneInfo.parentIndex;
        std::string abstractKey = mCachedBoneToKey[i];

        XMMATRIX bindLocal = XMLoadFloat4x4(&boneInfo.bindLocal);
        XMVECTOR S_bind, R_bind, T_bind;
        XMMatrixDecompose(&S_bind, &R_bind, &T_bind, bindLocal);

        XMVECTOR S_anim = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
        XMVECTOR R_anim = XMQuaternionIdentity();
        XMVECTOR T_anim = T_bind;

        bool isRootMotionBone = (parentIdx == -1) || (abstractKey == "Hips");

        for (auto& layer : mLayers)
        {
            float maskWeight = layer.GetCachedMaskWeight((int)i);
            float finalWeight = layer.GetWeight() * maskWeight;

            if (finalWeight <= 0.001f) continue;

            if (isRootMotionBone)
            {
                layer.EvaluateAndBlend((int)i, finalWeight, S_anim, R_anim, T_anim);
            }
            else
            {
                XMVECTOR tempS = S_anim;
                XMVECTOR tempR = R_anim;
                XMVECTOR tempT = T_anim;

                layer.EvaluateAndBlend((int)i, finalWeight, tempS, tempR, tempT);

                S_anim = tempS;
                R_anim = tempR;
            }
        }

        XMMATRIX finalGlobalTransform;

        if (!abstractKey.empty())
        {
            int logicalParentIdx = -1;
            int curr = parentIdx;
            while (curr >= 0)
            {
                if (!mCachedBoneToKey[curr].empty())
                {
                    logicalParentIdx = curr;
                    break;
                }
                curr = bones[curr].parentIndex;
            }

            XMMATRIX matAnimLocal = XMMatrixScalingFromVector(S_anim) * XMMatrixRotationQuaternion(R_anim) * XMMatrixTranslationFromVector(T_anim);

            XMMATRIX matLogicalParentGlobal = XMMatrixIdentity();
            if (logicalParentIdx >= 0)
            {
                matLogicalParentGlobal = globalTransforms[logicalParentIdx];
            }

            finalGlobalTransform = matAnimLocal * matLogicalParentGlobal;
        }
        else
        {
            XMMATRIX matParentGlobal = XMMatrixIdentity();
            if (parentIdx >= 0)
            {
                matParentGlobal = globalTransforms[parentIdx];
            }

            XMMATRIX matLocal = XMMatrixScalingFromVector(S_bind) * XMMatrixRotationQuaternion(R_bind) * XMMatrixTranslationFromVector(T_bind);

            finalGlobalTransform = matLocal * matParentGlobal;
        }

        globalTransforms[i] = finalGlobalTransform;
    }

    for (size_t i = 0; i < boneCount; ++i)
    {
        XMMATRIX invBind = XMLoadFloat4x4(&bones[i].inverseBind);
        XMMATRIX finalMat = XMMatrixTranspose(invBind * globalTransforms[i]);
        XMStoreFloat4x4(&mCpuBoneMatrices[i].transform, finalMat);
    }
}