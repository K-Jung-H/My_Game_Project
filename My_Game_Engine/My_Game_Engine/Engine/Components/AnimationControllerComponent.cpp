#include "AnimationControllerComponent.h"
#include "Core/Object.h"
#include "Resource/Model.h"
#include "GameEngine.h"
#include "DX_Graphics/ResourceUtils.h"


AnimationControllerComponent::AnimationControllerComponent()
    : Component()
{
}

void AnimationControllerComponent::CreateBoneMatrixBuffer()
{
    if (!mModelSkeleton || mBoneMatrixBuffer) return;

    const size_t boneCount = mModelSkeleton->BoneList.size();
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
}

void AnimationControllerComponent::SetModelAvatar(std::shared_ptr<Model_Avatar> model_avatar)
{
    mModelAvatar = model_avatar;
}

bool AnimationControllerComponent::IsReady() const
{
    return mModelSkeleton != nullptr && mModelAvatar != nullptr && mBoneMatrixSRVSlot != UINT_MAX;
}

void AnimationControllerComponent::Play(std::shared_ptr<AnimationClip> clip, bool isLooping)
{
    if (!clip)
        return;

    mCurrentClip = clip;
    mIsLooping = isLooping;
    mCurrentTime = 0.0f;

    mClipAvatar = clip->GetAvatar();
    mClipSkeleton = clip->GetSkeleton();

    if (!mClipAvatar || !mClipSkeleton || !mModelAvatar || !mModelSkeleton)
    {
        OutputDebugStringA("[AnimationControllerComponent] Error: 필요한 Avatar 또는 Skeleton 부재.\n");
        mCurrentClip = nullptr;
        return;
    }

    if (mClipAvatar->GetDefinitionType() != mModelAvatar->GetDefinitionType())
    {
        OutputDebugStringA("[AnimationControllerComponent] Warning: DefinitionType 불일치.\n");
        mCurrentClip = nullptr;
        return;
    }

    mMappedClipBoneIndex.clear();
    mMappedModelBoneIndex.clear();

    const auto& modelMap = mModelAvatar->GetBoneMap();
    const auto& clipMap = mClipAvatar->GetBoneMap();

    for (const auto& [abstractKey, modelBoneName] : modelMap)
    {
        auto itClip = clipMap.find(abstractKey);
        if (itClip == clipMap.end())
            continue;

        const std::string& clipBoneName = itClip->second;

        int clipIndex = mClipSkeleton->GetBoneIndex(clipBoneName);
        int modelIndex = mModelSkeleton->GetBoneIndex(modelBoneName);

        if (clipIndex < 0 || modelIndex < 0)
            continue;

        mMappedClipBoneIndex[abstractKey] = clipIndex;
        mMappedModelBoneIndex[abstractKey] = modelIndex;
    }

    OutputDebugStringA("[AnimationControllerComponent] Play(): Bone 매핑 완료.\n");
}


void AnimationControllerComponent::Update(float deltaTime)
{
    if (!IsReady())
    {
        return;
    }

    if (mCurrentClip)
    {
        mCurrentTime += deltaTime;

        float duration = mCurrentClip->GetDuration();
        if (duration > 0.0f)
        {
            if (mCurrentTime > duration)
            {
                if (mIsLooping)
                {
                    mCurrentTime = fmod(mCurrentTime, duration);
                }
                else
                {
                    mCurrentTime = duration;
                }
            }
        }

        EvaluateAnimation(mCurrentTime);
    }
    else
    {
        EvaluateAnimation(0.0f);
    }

    if (mMappedBoneBuffer)
    {
        memcpy(mMappedBoneBuffer, mCpuBoneMatrices.data(), sizeof(BoneMatrixData) * mCpuBoneMatrices.size());
    }
}

void AnimationControllerComponent::EvaluateAnimation(float time)
{
    using namespace DirectX;

    if (!mModelSkeleton || !mModelAvatar) return;

    const size_t boneCount = mModelSkeleton->GetBoneCount();
    if (boneCount == 0) return;

    const auto& boneList = mModelSkeleton->GetBoneList();
    const auto& bindLocalArr = mModelSkeleton->GetBindLocalList();
    const auto& invBindArr = mModelSkeleton->GetInverseBindList();

    std::vector<XMMATRIX> localTransforms(boneCount);


    for (size_t i = 0; i < boneCount; ++i)
    {
        const Bone& bone = boneList[i];
        XMMATRIX bindLocal = XMLoadFloat4x4(&bindLocalArr[i]);

        std::string abstractKey;
        for (auto& [k, v] : mModelAvatar->GetBoneMap())
        {
            if (v == bone.name)
            {
                abstractKey = k;
                break;
            }
        }

        const AnimationTrack* track = nullptr;
        if (mCurrentClip && !abstractKey.empty())
            track = mCurrentClip->GetTrack(abstractKey);

        if (!track)
        {
            localTransforms[i] = bindLocal;
            continue;
        }

        XMVECTOR S_bind, R_bind, T_bind;
        XMMatrixDecompose(&S_bind, &R_bind, &T_bind, bindLocal);

        XMFLOAT4 r_anim_f = track->SampleRotation(time);
        XMVECTOR R_anim = XMLoadFloat4(&r_anim_f);
        XMMATRIX local =
            XMMatrixScalingFromVector(S_bind) *
            XMMatrixRotationQuaternion(R_anim) *
            XMMatrixTranslationFromVector(T_bind);

        localTransforms[i] = local;
    }

    std::vector<XMMATRIX> globalTransforms(boneCount);

    for (size_t i = 0; i < boneCount; ++i)
    {
        int parent = boneList[i].parentIndex;
        if (parent < 0)
            globalTransforms[i] = localTransforms[i];
        else
            globalTransforms[i] = localTransforms[i] * globalTransforms[parent];
    }

    for (size_t i = 0; i < boneCount; ++i)
    {
        XMMATRIX invBind = XMLoadFloat4x4(&invBindArr[i]);
        XMMATRIX final = XMMatrixTranspose(invBind * globalTransforms[i]);

        XMStoreFloat4x4(&mCpuBoneMatrices[i].transform, final);
    }
}