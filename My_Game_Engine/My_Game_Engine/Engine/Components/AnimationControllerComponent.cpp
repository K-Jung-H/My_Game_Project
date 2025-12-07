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

rapidjson::Value AnimationControllerComponent::ToJSON(rapidjson::Document::AllocatorType& alloc) const
{
    rapidjson::Value v(rapidjson::kObjectType);

    v.AddMember("type", "AnimationControllerComponent", alloc);
    v.AddMember("IsPaused", mIsPaused, alloc);

    std::string skelGUID = mModelSkeleton ? mModelSkeleton->GetGUID() : "";
    v.AddMember("SkeletonGUID", rapidjson::Value(skelGUID.c_str(), alloc), alloc);

    std::string avatarGUID = mModelAvatar ? mModelAvatar->GetGUID() : "";
    v.AddMember("ModelAvatarGUID", rapidjson::Value(avatarGUID.c_str(), alloc), alloc);

    rapidjson::Value layerArray(rapidjson::kArrayType);
    for (const auto& layer : mLayers)
    {
        layerArray.PushBack(layer.ToJSON(alloc), alloc);
    }
    v.AddMember("Layers", layerArray, alloc);

    return v;
}

void AnimationControllerComponent::FromJSON(const rapidjson::Value& val)
{
    auto resSystem = GameEngine::Get().GetResourceSystem();

    if (val.HasMember("IsPaused")) mIsPaused = val["IsPaused"].GetBool();

    if (val.HasMember("SkeletonGUID"))
    {
        std::string guid = val["SkeletonGUID"].GetString();
        if (!guid.empty())
        {
            auto skel = resSystem->GetByGUID<Skeleton>(guid);
            SetSkeleton(skel);
        }
    }

    if (val.HasMember("ModelAvatarGUID"))
    {
        std::string guid = val["ModelAvatarGUID"].GetString();
        if (!guid.empty())
        {
            auto avatar = resSystem->GetByGUID<Model_Avatar>(guid);
            SetModelAvatar(avatar);
        }
    }

    if (val.HasMember("Layers"))
    {
        const rapidjson::Value& layersVal = val["Layers"];
        if (layersVal.IsArray())
        {
            mLayers.clear();
            for (const auto& layerVal : layersVal.GetArray())
            {
                AnimationLayer layer;
                layer.FromJSON(layerVal);
                mLayers.push_back(layer);
            }
        }
    }
}

void AnimationControllerComponent::WakeUp()
{
    if (mModelSkeleton && !mBoneMatrixBuffer)
        CreateBoneMatrixBuffer();

    UpdateBoneMappingCache();
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
    const auto& bones = mModelSkeleton->GetBones();

    mCachedBoneToKey.clear();
    mCachedBoneToKey.resize(boneCount);

    mControllerBoneCache.clear();
    mControllerBoneCache.resize(boneCount);

    for (size_t i = 0; i < boneCount; ++i)
    {
        std::string boneName = mModelSkeleton->GetBoneName((int)i);
        mCachedBoneToKey[i] = mModelAvatar->GetMappedKeyByBoneName(boneName);

        ControllerBoneCache& cache = mControllerBoneCache[i];
        const BoneInfo& boneInfo = bones[i];


        cache.hasMapping = !mCachedBoneToKey[i].empty();
        cache.isRootMotion = (boneInfo.parentIndex == -1) || (mCachedBoneToKey[i] == "Hips");

        cache.bindScale = boneInfo.bindScale;
        cache.bindRotation = boneInfo.bindRotation;
        cache.bindTranslation = boneInfo.bindTranslation;

        int logicalParent = -1;
        if (cache.hasMapping)
        {
            int curr = boneInfo.parentIndex;
            while (curr >= 0)
            {
                if (!mCachedBoneToKey[curr].empty())
                {
                    logicalParent = curr;
                    break;
                }
                curr = bones[curr].parentIndex;
            }
        }
        else
        {
            logicalParent = boneInfo.parentIndex;
        }
        cache.logicalParentIdx = logicalParent;
        cache.globalTransform = XMMatrixIdentity();
    }

    for (auto& layer : mLayers)
    {
        auto mask = layer.GetMask();
        if (mask) mask->ExpandHierarchy(mModelSkeleton.get(), mCachedBoneToKey);

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

    const size_t boneCount = mControllerBoneCache.size();

    if (mCpuBoneMatrices.size() != boneCount)
        mCpuBoneMatrices.resize(boneCount);

    for (size_t i = 0; i < boneCount; ++i)
    {
        ControllerBoneCache& cache = mControllerBoneCache[i];

        XMVECTOR S_bind = XMLoadFloat3(&cache.bindScale);
        XMVECTOR R_bind = XMLoadFloat4(&cache.bindRotation);
        XMVECTOR T_bind = XMLoadFloat3(&cache.bindTranslation);

        XMVECTOR S_anim = S_bind;
        XMVECTOR R_anim = R_bind;
        XMVECTOR T_anim = T_bind;

        bool isRootMotionBone = cache.isRootMotion;

        for (auto& layer : mLayers)
        {
            if (isRootMotionBone)
            {
                layer.EvaluateAndBlend((int)i, S_anim, R_anim, T_anim);
            }
            else
            {
                XMVECTOR tempS = S_anim;
                XMVECTOR tempR = R_anim;
                XMVECTOR tempT = T_anim;

                if (layer.EvaluateAndBlend((int)i, tempS, tempR, tempT))
                {
                    S_anim = tempS;
                    R_anim = tempR;
                    // T_anim = tempT;
                }
            }
        }

        R_anim = XMQuaternionNormalize(R_anim);

        XMMATRIX finalGlobalTransform;

        if (cache.hasMapping)
        {
            XMMATRIX matAnimLocal = XMMatrixScalingFromVector(S_anim) * XMMatrixRotationQuaternion(R_anim) * XMMatrixTranslationFromVector(T_anim);

            XMMATRIX matLogicalParentGlobal = XMMatrixIdentity();

            int logicalParentIdx = cache.logicalParentIdx;
            if (logicalParentIdx >= 0)
            {
                matLogicalParentGlobal = mControllerBoneCache[logicalParentIdx].globalTransform;
            }

            finalGlobalTransform = matAnimLocal * matLogicalParentGlobal;
        }
        else
        {
            int parentIdx = cache.logicalParentIdx;

            XMMATRIX matParentGlobal = XMMatrixIdentity();
            if (parentIdx >= 0)
            {
                matParentGlobal = mControllerBoneCache[parentIdx].globalTransform;
            }

            XMMATRIX matLocal = XMMatrixScalingFromVector(S_bind) * XMMatrixRotationQuaternion(R_bind) * XMMatrixTranslationFromVector(T_bind);

            finalGlobalTransform = matLocal * matParentGlobal;
        }

        cache.globalTransform = finalGlobalTransform;
    }


    const auto& bones = mModelSkeleton->GetBones(); 
    for (size_t i = 0; i < boneCount; ++i)
    {
        XMMATRIX invBind = XMLoadFloat4x4(&bones[i].inverseBind);
        XMMATRIX finalMat = XMMatrixTranspose(invBind * mControllerBoneCache[i].globalTransform);

        XMStoreFloat4x4(&mCpuBoneMatrices[i].transform, finalMat);
    }
}