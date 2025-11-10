#include "AnimationControllerComponent.h"
#include "Core/Object.h"
#include "Resource/Model.h"
#include "GameEngine.h"
#include "DX_Graphics/ResourceUtils.h"

AnimationControllerComponent::AnimationControllerComponent()
    : Component()
{
}

void AnimationControllerComponent::SetSkeleton(std::shared_ptr<Skeleton> skeleton)
{
    if (mSkeleton == skeleton) return;

    mSkeleton = skeleton;

    if (mSkeleton && !mBoneMatrixBuffer)
    {
        CreateBoneMatrixBuffer();
    }
    else if (!mSkeleton)
    {
    }
}

void AnimationControllerComponent::CreateBoneMatrixBuffer()
{
    if (!mSkeleton || mBoneMatrixBuffer) return;

    const size_t boneCount = mSkeleton->BoneList.size();
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

    mBoneMatrixSRVSlot = heap->Allocate(HeapRegion::SRV_Frame);
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.NumElements = (UINT)boneCount;
    srvDesc.Buffer.StructureByteStride = sizeof(BoneMatrixData);
    rc.device->CreateShaderResourceView(mBoneMatrixBuffer.Get(), &srvDesc, heap->GetCpuHandle(mBoneMatrixSRVSlot));
}

void AnimationControllerComponent::Update(float deltaTime)
{
    if (!mSkeleton || !mMappedBoneBuffer) return;

    mCurrentTime += deltaTime;

    EvaluateAnimation(mCurrentTime);

    memcpy(mMappedBoneBuffer, mCpuBoneMatrices.data(), sizeof(BoneMatrixData) * mCpuBoneMatrices.size());
}

void AnimationControllerComponent::EvaluateAnimation(float time)
{
    using namespace DirectX;
    const size_t boneCount = mSkeleton->BoneList.size();
    if (boneCount == 0) return;


    std::vector<XMMATRIX> localTransforms(boneCount);
    std::vector<XMMATRIX> globalTransforms(boneCount);

    for (size_t i = 0; i < boneCount; ++i)
    {
        const Bone& bone = mSkeleton->BoneList[i];
        XMMATRIX invBind = XMLoadFloat4x4(&bone.inverseBind);
        XMMATRIX globalT_Pose = XMMatrixInverse(nullptr, invBind);

        int parentIdx = bone.parentIndex;
        if (parentIdx == -1)
        {
            localTransforms[i] = globalT_Pose;
        }
        else
        {

            const Bone& parentBone = mSkeleton->BoneList[parentIdx];
            XMMATRIX parentInvBind = XMLoadFloat4x4(&parentBone.inverseBind);
            XMMATRIX parentGlobalT_Pose = XMMatrixInverse(nullptr, parentInvBind);
            localTransforms[i] = globalT_Pose * XMMatrixInverse(nullptr, parentGlobalT_Pose);
        }

        if (bone.name == "Spine1")
        {
            XMMATRIX rotation = XMMatrixRotationZ(sin(time * 2.0f) * 0.5f);
            localTransforms[i] = rotation * localTransforms[i];
        }
    }

    for (size_t i = 0; i < boneCount; ++i)
    {
        int parentIdx = mSkeleton->BoneList[i].parentIndex;
        if (parentIdx == -1)
        {
            globalTransforms[i] = localTransforms[i]; 
        }
        else
        {
            globalTransforms[i] = localTransforms[i] * globalTransforms[parentIdx];
        }
    }

    for (size_t i = 0; i < boneCount; ++i)
    {
        XMMATRIX invBind = XMLoadFloat4x4(&mSkeleton->BoneList[i].inverseBind);

        XMMATRIX final = XMMatrixTranspose(invBind * globalTransforms[i]);
        XMStoreFloat4x4(&mCpuBoneMatrices[i].transform, final);
    }
}
UINT AnimationControllerComponent::GetBoneMatrixSRV() const
{
    return mBoneMatrixSRVSlot;
}