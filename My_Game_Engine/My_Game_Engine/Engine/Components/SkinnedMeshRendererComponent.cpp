#include "SkinnedMeshRendererComponent.h"
#include "AnimationControllerComponent.h"
#include "Core/Object.h" 
#include "GameEngine.h"
#include "DX_Graphics/ResourceUtils.h"

SkinnedMeshRendererComponent::SkinnedMeshRendererComponent()
    : MeshRendererComponent()
{
}

rapidjson::Value SkinnedMeshRendererComponent::ToJSON(rapidjson::Document::AllocatorType& alloc) const
{
    rapidjson::Value v(rapidjson::kObjectType);

    v.AddMember("type", "SkinnedMeshRendererComponent", alloc);

    auto mesh = GetMesh();
    std::string meshGUID = mesh ? mesh->GetGUID() : "";
    v.AddMember("MeshGUID", rapidjson::Value(meshGUID.c_str(), alloc), alloc);

    if (mesh)
    {
        rapidjson::Value matArray(rapidjson::kArrayType);
        UINT count = mesh->GetSubMeshCount();
        for (UINT i = 0; i < count; ++i)
        {
            UINT matId = GetMaterial(i);
            auto mat = GameEngine::Get().GetResourceSystem()->GetById<Material>(matId);
            std::string matGUID = mat ? mat->GetGUID() : "";
            matArray.PushBack(rapidjson::Value(matGUID.c_str(), alloc), alloc);
        }
        v.AddMember("Materials", matArray, alloc);
    }

    return v;
}

void SkinnedMeshRendererComponent::FromJSON(const rapidjson::Value& val)
{
    auto resSystem = GameEngine::Get().GetResourceSystem();

    if (val.HasMember("MeshGUID"))
    {
        std::string guid = val["MeshGUID"].GetString();
        if (!guid.empty())
        {
            auto mesh = resSystem->GetByGUID<Mesh>(guid);
            if (mesh)
            {
                SetMesh(mesh->GetId());
            }
        }
    }

    if (val.HasMember("Materials"))
    {
        const auto& mats = val["Materials"];
        if (mats.IsArray())
        {
            for (rapidjson::SizeType i = 0; i < mats.Size(); ++i)
            {
                std::string guid = mats[i].GetString();
                if (!guid.empty())
                {
                    auto mat = resSystem->GetByGUID<Material>(guid);
                    if (mat)
                    {
                        SetMaterial(i, mat->GetId());
                    }
                }
            }
        }
    }
}


void SkinnedMeshRendererComponent::Initialize()
{
    CacheAnimController();
}

void SkinnedMeshRendererComponent::CacheAnimController()
{
    Object* current = GetOwner();
    if (!current) return;

    while (current->GetParent() != nullptr)
    {
        current = current->GetParent();
    }

    mCachedAnimController = current->GetComponent<AnimationControllerComponent>();

    if (!mCachedAnimController)
    {
        OutputDebugStringA("[SkinnedMeshRendererComponent] Warning: 루트에서 AnimationController를 찾지 못했습니다.\n");
    }
}

void SkinnedMeshRendererComponent::SetMesh(UINT id)
{
    mDeferredMeshId = id;
    mDeferredMeshUpdate = true;
}

void SkinnedMeshRendererComponent::Update()
{
    if (mDeferredMeshUpdate)
    {
        ApplyDeferredMeshChange();
    }
}

void SkinnedMeshRendererComponent::ApplyDeferredMeshChange()
{
    GameEngine::Get().GetRenderer()->FlushCommandQueue();

    MeshRendererComponent::SetMesh(mDeferredMeshId);

    if (auto mesh = GetMesh())
    {
        auto skinnedMesh = std::dynamic_pointer_cast<SkinnedMesh>(mesh);
        if (skinnedMesh)
        {
            mHasSkinnedBuffer = false;
            CreatePreSkinnedOutputBuffers(skinnedMesh);
            mOriginalHotVBV = skinnedMesh->GetHotVBV();
        }
        else
        {
            mHasSkinnedBuffer = false;
        }
    }
    else
    {
        mHasSkinnedBuffer = false;
    }

    mDeferredMeshUpdate = false;
}

void SkinnedMeshRendererComponent::CreatePreSkinnedOutputBuffers(std::shared_ptr<SkinnedMesh> skinnedMesh)
{
    if (mHasSkinnedBuffer || !skinnedMesh || !skinnedMesh->GetHotVB())
    {
        return;
    }

    const D3D12_VERTEX_BUFFER_VIEW& hotVBV = skinnedMesh->GetHotVBV();
    const UINT bytes = hotVBV.SizeInBytes;
    const UINT stride = hotVBV.StrideInBytes;

    if (bytes == 0 || stride == 0)
    {
        OutputDebugStringA("[SkinnedMeshRendererComponent] Error: 원본 HotVBV 정보가 유효하지 않습니다.\n");
        return;
    }

    RendererContext rc = GameEngine::Get().Get_UploadContext();
    DescriptorManager* heap = rc.resourceHeap;

    for (UINT i = 0; i < Engine::Frame_Render_Buffer_Count; ++i)
    {
        FrameSkinBuffer& fsb = mFrameSkinnedBuffers[i];

        fsb.skinnedBuffer = ResourceUtils::CreateBufferResource(
            rc, nullptr, bytes,
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            fsb.uploadBuffer
        );

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.NumElements = bytes / stride;
        uavDesc.Buffer.StructureByteStride = stride;

        fsb.uavSlot = heap->Allocate(HeapRegion::UAV);
        rc.device->CreateUnorderedAccessView(fsb.skinnedBuffer.Get(), nullptr, &uavDesc, heap->GetCpuHandle(fsb.uavSlot));

        fsb.vbv.BufferLocation = fsb.skinnedBuffer->GetGPUVirtualAddress();
        fsb.vbv.StrideInBytes = stride;
        fsb.vbv.SizeInBytes = bytes;

        fsb.mIsSkinningResultReady = false;
    }

    mHasSkinnedBuffer = true;
}

const D3D12_VERTEX_BUFFER_VIEW& SkinnedMeshRendererComponent::GetSkinnedVBV(UINT frameIndex) const
{
    if (mHasSkinnedBuffer && mFrameSkinnedBuffers[frameIndex].mIsSkinningResultReady)
        return mFrameSkinnedBuffers[frameIndex].vbv;

    return mOriginalHotVBV;
}

bool SkinnedMeshRendererComponent::HasValidBuffers() const
{
    if (!mHasSkinnedBuffer)
    {
        return false;
    }

    if (!mCachedAnimController)
    {
        return false;
    }

    return mCachedAnimController->IsReady();
}