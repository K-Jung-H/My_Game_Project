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

        // 1. Bind Pose 정보 가져오기
        XMMATRIX bindLocal = XMLoadFloat4x4(&boneInfo.bindLocal);
        XMVECTOR S_bind, R_bind, T_bind;
        XMMatrixDecompose(&S_bind, &R_bind, &T_bind, bindLocal);

        // 2. 애니메이션 적용 변수 초기화
        XMVECTOR S_anim = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
        XMVECTOR R_anim = XMQuaternionIdentity();
        XMVECTOR T_anim = T_bind; // 위치는 Bind Pose 유지 (Bone Length 보존)

        // Hips(Root) 판별
        bool isRootMotionBone = (parentIdx == -1) || (abstractKey == "Hips");

        // 3. 애니메이션 블렌딩 (매핑된 뼈만 영향 받음)
        for (auto& layer : mLayers)
        {
            float weight = layer.GetWeight();
            if (weight <= 0.001f) continue;

            float maskWeight = 1.0f;
            auto mask = layer.GetMask();
            if (mask && !abstractKey.empty())
            {
                maskWeight = mask->GetWeight(abstractKey);
            }

            float finalWeight = layer.GetWeight() * maskWeight;
            if (finalWeight <= 0.001f) continue;

            // 매핑된 뼈는 여기서 R_anim이 애니메이션 값으로 갱신됨
            if (isRootMotionBone)
            {
                layer.EvaluateAndBlend(abstractKey, finalWeight, S_anim, R_anim, T_anim);
            }
            else
            {
                // 위치(T)는 덮어쓰지 않도록 임시 변수 사용
                XMVECTOR tempS = S_anim;
                XMVECTOR tempR = R_anim;
                XMVECTOR tempT = T_anim;

                layer.EvaluateAndBlend(abstractKey, finalWeight, tempS, tempR, tempT);

                S_anim = tempS;
                R_anim = tempR;
                // T_anim은 변경 안 함 (T_bind 유지)
            }
        }

        XMMATRIX finalGlobalTransform;

        if (!abstractKey.empty())
        {
            // ========================================================
            // Case A: 매핑된 뼈 (Animation Driven)
            // ========================================================
            // 가상 부모(Virtual Parent) 로직 적용

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

            // 내 애니메이션 로컬 행렬
            XMMATRIX matAnimLocal = XMMatrixScalingFromVector(S_anim) * XMMatrixRotationQuaternion(R_anim) * XMMatrixTranslationFromVector(T_anim);

            // 논리적 부모의 월드 행렬
            XMMATRIX matLogicalParentGlobal = XMMatrixIdentity();
            if (logicalParentIdx >= 0)
            {
                matLogicalParentGlobal = globalTransforms[logicalParentIdx];
            }

            finalGlobalTransform = matAnimLocal * matLogicalParentGlobal;
        }
        else
        {
            // ========================================================
            // Case B: 매핑 안 된 뼈 (Bind Pose Driven)
            // ========================================================
            // [수정] 회전값을 Identity(0)로 강제하지 않고, 원래 Bind Pose의 회전값(R_bind)을 사용합니다.
            // 이렇게 해야 원래 굽혀져 있던 장식 뼈나 Roll 본의 각도가 유지됩니다.

            XMMATRIX matParentGlobal = XMMatrixIdentity();
            if (parentIdx >= 0)
            {
                matParentGlobal = globalTransforms[parentIdx];
            }

            // S_bind, R_bind, T_bind를 사용하여 원래 모양 유지
            // (S_anim 대신 S_bind를 쓰는 것이 더 안전할 수 있음)
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