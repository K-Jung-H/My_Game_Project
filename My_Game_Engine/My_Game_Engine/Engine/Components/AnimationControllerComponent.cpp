#include "AnimationControllerComponent.h"
#include "Core/Object.h"
#include "Resource/Model.h"
#include "GameEngine.h"
#include "DX_Graphics/ResourceUtils.h"

using namespace DirectX;

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

void AnimationControllerComponent::Update(float deltaTime)
{
    if (!IsReady()) return;

    for (auto& layer : mLayers)
    {
        layer.Update(deltaTime);
    }

    EvaluateLayers();

    if (mMappedBoneBuffer)
    {
        memcpy(mMappedBoneBuffer, mCpuBoneMatrices.data(), sizeof(BoneMatrixData) * mCpuBoneMatrices.size());
    }
}

//void AnimationControllerComponent::EvaluateLayers()
//{
//    if (!mModelSkeleton || !mModelAvatar) return;
//
//    const auto& bones = mModelSkeleton->GetBones();
//    const size_t boneCount = bones.size();
//
//    if (mCpuBoneMatrices.size() != boneCount)
//    {
//        mCpuBoneMatrices.resize(boneCount);
//    }
//
//    std::vector<XMMATRIX> localTransforms(boneCount);
//
//    for (size_t i = 0; i < boneCount; ++i)
//    {
//        const std::string& abstractKey = mCachedBoneToKey[i];
//        const BoneInfo& boneInfo = bones[i];
//
//        XMMATRIX bindLocal = XMLoadFloat4x4(&boneInfo.bindLocal);
//
//        XMVECTOR S_acc, R_acc, T_acc;
//        XMMatrixDecompose(&S_acc, &R_acc, &T_acc, bindLocal);
//
//        for (auto& layer : mLayers)
//        {
//            float layerWeight = layer.GetWeight();
//            if (layerWeight <= 0.001f) continue;
//
//            float maskWeight = 1.0f;
//            auto mask = layer.GetMask();
//            if (mask && !abstractKey.empty())
//            {
//                maskWeight = mask->GetWeight(abstractKey);
//            }
//
//            float finalWeight = layerWeight * maskWeight;
//            if (finalWeight <= 0.001f) continue;
//
//            layer.EvaluateAndBlend(abstractKey, finalWeight, S_acc, R_acc, T_acc);
//        }
//
//        localTransforms[i] =
//            XMMatrixScalingFromVector(S_acc) * XMMatrixRotationQuaternion(R_acc) * XMMatrixTranslationFromVector(T_acc);
//    }
//
//    std::vector<XMMATRIX> globalTransforms(boneCount);
//
//    for (size_t i = 0; i < boneCount; ++i)
//    {
//        int parent = bones[i].parentIndex;
//        if (parent < 0)
//            globalTransforms[i] = localTransforms[i];
//        else
//            globalTransforms[i] = localTransforms[i] * globalTransforms[parent];
//    }
//
//    for (size_t i = 0; i < boneCount; ++i)
//    {
//        XMMATRIX invBind = XMLoadFloat4x4(&bones[i].inverseBind);
//        XMMATRIX finalMat = XMMatrixTranspose(invBind * globalTransforms[i]);
//        XMStoreFloat4x4(&mCpuBoneMatrices[i].transform, finalMat);
//    }
//}

void AnimationControllerComponent::EvaluateLayers()
{
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

        XMVECTOR S_acc, R_acc, T_acc;
        XMMatrixDecompose(&S_acc, &R_acc, &T_acc, bindLocal);

        XMVECTOR R_correction = XMQuaternionIdentity();
        if (!abstractKey.empty())
        {
            XMFLOAT4 corr = mModelAvatar->GetCorrection(abstractKey);
            R_correction = XMLoadFloat4(&corr);
        }

        R_acc = XMQuaternionMultiply(R_acc, R_correction);

        for (auto& layer : mLayers)
        {
            float layerWeight = layer.GetWeight();
            if (layerWeight <= 0.001f) continue;

            float maskWeight = 1.0f;
            auto mask = layer.GetMask();
            if (mask && !abstractKey.empty())
            {
                maskWeight = mask->GetWeight(abstractKey);
            }

            float finalWeight = layerWeight * maskWeight;
            if (finalWeight <= 0.001f) continue;

            layer.EvaluateAndBlend(abstractKey, finalWeight, S_acc, R_acc, T_acc);
        }

        localTransforms[i] =
            XMMatrixScalingFromVector(S_acc) * XMMatrixRotationQuaternion(R_acc) * XMMatrixTranslationFromVector(T_acc);
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