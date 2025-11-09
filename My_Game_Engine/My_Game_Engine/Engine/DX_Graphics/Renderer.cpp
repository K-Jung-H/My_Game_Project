#include "Renderer.h"
#include "GameEngine.h"
#include "Core/Object.h"
#include "DXMathUtils.h"
#include "ResourceUtils.h"
#include "Resource/Mesh.h"
#include "Resource/Material.h"
#include "Components/RigidbodyComponent.h"
#include <stdexcept>


void ResourceStateTracker::Register(ID3D12Resource* resource, D3D12_RESOURCE_STATES initialState)
{
    mCurrentStates[resource] = initialState;
}

void ResourceStateTracker::Transition(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* resource, D3D12_RESOURCE_STATES newState)
{
    auto it = mCurrentStates.find(resource);
    D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;

    if (it != mCurrentStates.end())
        currentState = it->second;
    else
        mCurrentStates[resource] = currentState;

    if (currentState != newState)
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, currentState, newState);
        cmdList->ResourceBarrier(1, &barrier);

        mCurrentStates[resource] = newState;
    }
}

void ResourceStateTracker::UAVBarrier(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* resource)
{
    auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(resource);
    cmdList->ResourceBarrier(1, &barrier);
}

//=======================================================================

float DX12_Renderer::clear_color[4] = { 0.95f, 0.55f, 0.60f, 1.00f };

bool DX12_Renderer::Initialize(HWND m_hWnd, UINT width, UINT height)
{

#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();

        ComPtr<ID3D12Debug1> debugController1;
        if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1))))
        {
            debugController1->SetEnableGPUBasedValidation(TRUE);
            OutputDebugStringA("[D3D12] GPU-based validation enabled.\n");
        }
    }
#endif

    mWidth = width;
    mHeight = height;

    bool success = true;

    success &= CreateDeviceAndFactory();
    success &= CheckMsaaSupport();
    success &= CreateCommandQueue();
    success &= CreateSwapChain(m_hWnd, width, height);
    success &= CreateDescriptorHeaps();
    success &= CreateFrameResources();
    success &= CreateCommandList();
    success &= CreateFenceObjects();
    success &= CreateCommandList_Upload();
    success &= Create_Shader();
    success &= Create_SceneCBV();

    //---------------------------------------------------------------------
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1; 
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;

    if (FAILED(mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mImguiSrvHeap))))
        return false;


    is_initialized = true;

    return true;
}

bool DX12_Renderer::CreateDeviceAndFactory()
{
    UINT dxgiFactoryFlags = 0;


    // Factory
    if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory))))
        return false;

    // Search Hardware Adapter
    ComPtr<IDXGIAdapter1> adapter;

    for (UINT i = 0; mFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue; // exclude software adapter

        // 디바이스 생성 시도
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice))))
        {
            adapter.As(&mAdapter);
            break;
        }
    }

    // if fail finding adapter WARP fallback
    if (!mDevice)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        if (FAILED(mFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter))))
            return false;

        if (FAILED(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice))))
            return false;

        warpAdapter.As(&mAdapter);
    }

    return true;
}

bool DX12_Renderer::CheckMsaaSupport()
{
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaLevels = {};
    msaaLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    msaaLevels.SampleCount = 4;
    msaaLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;

    if (FAILED(mDevice->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msaaLevels,
        sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS))))
        return false;

    mMsaa4xQualityLevels = msaaLevels.NumQualityLevels;
    mEnableMsaa4x = (mMsaa4xQualityLevels > 0);
    return true;
}


bool DX12_Renderer::CreateCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    return SUCCEEDED(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
}

bool DX12_Renderer::CreateSwapChain(HWND m_hWnd, UINT width, UINT height)
{
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.BufferCount = FrameCount;
    desc.Width = width;
    desc.Height = height;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    if (FAILED(mFactory->CreateSwapChainForHwnd(
        mCommandQueue.Get(), m_hWnd, &desc, nullptr, nullptr, &swapChain)))
        return false;

    mFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);

    swapChain.As(&mSwapChain);
    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
    return true;
}

bool DX12_Renderer::CreateRTVHeap()
{
    UINT rtvCount = FrameCount + FrameCount * 2 + FrameCount * (UINT)GBufferType::Count; // BackBuffer_RT: 1, CompositeBuffer_RT: 2, G-Buffer_RT: N * FrameCount
    mRtvManager = std::make_unique<DescriptorManager>(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, rtvCount);
    return true;
}

bool DX12_Renderer::CreateDSVHeap()
{
    UINT dsvCountPerFrame = 1 + MAX_SHADOW_SPOT + (MAX_SHADOW_CSM * NUM_CSM_CASCADES)+ (MAX_SHADOW_POINT * 6);
    UINT totalDsvCount = dsvCountPerFrame * FrameCount;
    mDsvManager = std::make_unique<DescriptorManager>(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, totalDsvCount);
    return true;
}

bool DX12_Renderer::CreateResourceHeap()
{
    mResource_Heap_Manager = std::make_unique<DescriptorManager>(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, MAX_RESOURCE_HEAP_SIZE);
    return true;
}


bool DX12_Renderer::CreateDescriptorHeaps()
{
    if (!CreateRTVHeap()) return false;
    if (!CreateDSVHeap()) return false;
    if(!CreateResourceHeap()) return false;
    return true;
}


bool DX12_Renderer::CreateFrameResources()
{
    bool success = true;

    for (UINT i = 0; i < FrameCount; ++i)
    {
        FrameResource& fr = mFrameResources[i];

        CreateSingleFrameResource(fr, i);

        if (!success) return false;
    }

    return success;
}

bool DX12_Renderer::CreateSingleFrameResource(FrameResource& fr, UINT frameIndex)
{
    bool success = true;
    success &= CreateCommandAllocator(fr);
    success &= CreateBackBufferRTV(frameIndex, fr); 
    success &= CreateDSV(fr);           
    success &= CreateGBuffer(fr);       
    success &= Create_Merge_RenderTargets(fr); 
    success &= CreateObjectCB(fr, 5000);
    success &= Create_LightResources(fr, 500);
    success &= Create_ShadowResources(fr);
	success &= CreateShadowMatrixBuffer(fr);

    return success;
}

void DX12_Renderer::DestroyFrameResources()
{
    for (UINT i = 0; i < FrameCount; ++i)
    {
        FrameResource& fr = mFrameResources[i];

        DestroySingleFrameResource(fr);
    }
}

void DX12_Renderer::DestroySingleFrameResource(FrameResource& fr)
{
    if (fr.BackBufferRtvSlot_ID != UINT_MAX)
    {
        mRtvManager->FreeDeferred(HeapRegion::RTV, fr.BackBufferRtvSlot_ID);
        fr.BackBufferRtvSlot_ID = UINT_MAX;
    }
    if (fr.DsvSlot_ID != UINT_MAX)
    {
        mDsvManager->FreeDeferred(HeapRegion::DSV, fr.DsvSlot_ID);
        fr.DsvSlot_ID = UINT_MAX;
    }

    for (UINT slot : fr.GBufferRtvSlot_IDs)
        mRtvManager->FreeDeferred(HeapRegion::RTV, slot);
    fr.GBufferRtvSlot_IDs.clear();

    for (UINT slot : fr.GBufferSrvSlot_IDs)
        mResource_Heap_Manager->FreeDeferred(HeapRegion::SRV_Frame, slot);
    fr.GBufferSrvSlot_IDs.clear();

    for (int i = 0; i < 2; ++i)
    {
        if (fr.MergeRtvSlot_IDs[i] != UINT_MAX)
        {
            mRtvManager->FreeDeferred(HeapRegion::RTV, fr.MergeRtvSlot_IDs[i]);
            fr.MergeRtvSlot_IDs[i] = UINT_MAX;
        }
        if (fr.MergeSrvSlot_IDs[i] != UINT_MAX)
        {
            mResource_Heap_Manager->FreeDeferred(HeapRegion::SRV_Frame, fr.MergeSrvSlot_IDs[i]);
            fr.MergeSrvSlot_IDs[i] = UINT_MAX;
        }
    }

    if (fr.DepthBufferSrvSlot_ID != UINT_MAX)
    {
        mResource_Heap_Manager->FreeDeferred(HeapRegion::SRV_Frame, fr.DepthBufferSrvSlot_ID);
        fr.DepthBufferSrvSlot_ID = UINT_MAX;
    }

    if (fr.light_resource.ClusterBuffer_SRV_Index != UINT_MAX)
    {
        mResource_Heap_Manager->FreeDeferred(HeapRegion::SRV_Frame, fr.light_resource.ClusterBuffer_SRV_Index);
        fr.light_resource.ClusterBuffer_SRV_Index = UINT_MAX;
    }
    if (fr.light_resource.ClusterBuffer_UAV_Index != UINT_MAX)
    {
        mResource_Heap_Manager->FreeDeferred(HeapRegion::UAV, fr.light_resource.ClusterBuffer_UAV_Index);
        fr.light_resource.ClusterBuffer_UAV_Index = UINT_MAX;
    }
    if (fr.light_resource.LightBuffer_SRV_Index != UINT_MAX)
    {
        mResource_Heap_Manager->FreeDeferred(HeapRegion::SRV_Frame, fr.light_resource.LightBuffer_SRV_Index);
        fr.light_resource.LightBuffer_SRV_Index = UINT_MAX;
    }
    if (fr.light_resource.ClusterLightMetaBuffer_SRV_Index != UINT_MAX)
    {
        mResource_Heap_Manager->FreeDeferred(HeapRegion::SRV_Frame, fr.light_resource.ClusterLightMetaBuffer_SRV_Index);
        fr.light_resource.ClusterLightMetaBuffer_SRV_Index = UINT_MAX;
    }
    if (fr.light_resource.ClusterLightMetaBuffer_UAV_Index != UINT_MAX)
    {
        mResource_Heap_Manager->FreeDeferred(HeapRegion::UAV, fr.light_resource.ClusterLightMetaBuffer_UAV_Index);
        fr.light_resource.ClusterLightMetaBuffer_UAV_Index = UINT_MAX;
    }
    if (fr.light_resource.ClusterLightIndicesBuffer_SRV_Index != UINT_MAX)
    {
        mResource_Heap_Manager->FreeDeferred(HeapRegion::SRV_Frame, fr.light_resource.ClusterLightIndicesBuffer_SRV_Index);
        fr.light_resource.ClusterLightIndicesBuffer_SRV_Index = UINT_MAX;
    }
    if (fr.light_resource.ClusterLightIndicesBuffer_UAV_Index != UINT_MAX)
    {
        mResource_Heap_Manager->FreeDeferred(HeapRegion::UAV, fr.light_resource.ClusterLightIndicesBuffer_UAV_Index);
        fr.light_resource.ClusterLightIndicesBuffer_UAV_Index = UINT_MAX;
    }
    if (fr.light_resource.GlobalOffsetCounterBuffer_UAV_Index != UINT_MAX)
    {
        mResource_Heap_Manager->FreeDeferred(HeapRegion::UAV, fr.light_resource.GlobalOffsetCounterBuffer_UAV_Index);
        fr.light_resource.GlobalOffsetCounterBuffer_UAV_Index = UINT_MAX;
    }

    if (fr.light_resource.SpotShadowArray_SRV != UINT_MAX)
    {
        mResource_Heap_Manager->FreeDeferred(HeapRegion::SRV_ShadowMap_Spot, fr.light_resource.SpotShadowArray_SRV);
        fr.light_resource.SpotShadowArray_SRV = UINT_MAX;
    }
    if (fr.light_resource.CsmShadowArray_SRV != UINT_MAX)
    {
        mResource_Heap_Manager->FreeDeferred(HeapRegion::SRV_ShadowMap_CSM, fr.light_resource.CsmShadowArray_SRV);
        fr.light_resource.CsmShadowArray_SRV = UINT_MAX;
    }
    if (fr.light_resource.PointShadowCubeArray_SRV != UINT_MAX)
    {
        mResource_Heap_Manager->FreeDeferred(HeapRegion::SRV_ShadowMap_Point, fr.light_resource.PointShadowCubeArray_SRV);
        fr.light_resource.PointShadowCubeArray_SRV = UINT_MAX;
    }

    if (fr.light_resource.MappedShadowMatrixBuffer)
    {
        fr.light_resource.ShadowMatrixBuffer->Unmap(0, nullptr);
        fr.light_resource.MappedShadowMatrixBuffer = nullptr;
    }
    fr.light_resource.ShadowMatrixBuffer.Reset();

    if (fr.light_resource.ShadowMatrixBuffer_SRV_Index != UINT_MAX)
    {
        mResource_Heap_Manager->FreeDeferred(HeapRegion::SRV_Frame, fr.light_resource.ShadowMatrixBuffer_SRV_Index);
        fr.light_resource.ShadowMatrixBuffer_SRV_Index = UINT_MAX;
    }

    for (UINT dsvIndex : fr.light_resource.SpotShadow_DSVs)
        mDsvManager->FreeDeferred(HeapRegion::DSV, dsvIndex);
    fr.light_resource.SpotShadow_DSVs.clear();
    for (UINT dsvIndex : fr.light_resource.CsmShadow_DSVs)
        mDsvManager->FreeDeferred(HeapRegion::DSV, dsvIndex);
    fr.light_resource.CsmShadow_DSVs.clear();
    for (UINT dsvIndex : fr.light_resource.PointShadow_DSVs)
        mDsvManager->FreeDeferred(HeapRegion::DSV, dsvIndex);
    fr.light_resource.PointShadow_DSVs.clear();

    fr.DepthStencilBuffer.Reset();
    for (auto& target : fr.gbuffer.targets)
        target.Reset();
    fr.gbuffer.Depth.Reset();
    fr.Merge_RenderTargets[0].Reset();
    fr.Merge_RenderTargets[1].Reset();

    if (fr.ObjectCB.MappedObjectCB)
    {
        fr.ObjectCB.Buffer->Unmap(0, nullptr);
        fr.ObjectCB.MappedObjectCB = nullptr;
    }
    fr.ObjectCB.Buffer.Reset();

    if (fr.light_resource.MappedLightUploadBuffer)
    {
        fr.light_resource.LightUploadBuffer->Unmap(0, nullptr);
        fr.light_resource.MappedLightUploadBuffer = nullptr;
    }
    fr.light_resource.LightUploadBuffer.Reset();
    fr.light_resource.LightBuffer.Reset();
    fr.light_resource.ClusterBuffer.Reset();
    fr.light_resource.ClusterLightMetaBuffer.Reset();
    fr.light_resource.ClusterLightIndicesBuffer.Reset();
    fr.light_resource.GlobalOffsetCounterBuffer.Reset();

    fr.light_resource.SpotShadowArray.Reset();
    fr.light_resource.CsmShadowArray.Reset();
    fr.light_resource.PointShadowCubeArray.Reset();

    fr.CommandAllocator.Reset();

    fr.FenceValue = 0;

    fr.StateTracker.Clear();
}
bool DX12_Renderer::CreateCommandAllocator(FrameResource& fr)
{
    return SUCCEEDED(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&fr.CommandAllocator)));
}

bool DX12_Renderer::CreateBackBufferRTV(UINT frameIndex, FrameResource& fr)
{
    if (FAILED(mSwapChain->GetBuffer(frameIndex, IID_PPV_ARGS(&fr.RenderTarget))))
        return false;

    UINT Slot = mRtvManager->Allocate(HeapRegion::RTV);

    fr.BackBufferRtvSlot_ID = Slot;

    mDevice->CreateRenderTargetView(fr.RenderTarget.Get(), nullptr, mRtvManager->GetCpuHandle(Slot));

    fr.StateTracker.Register(fr.RenderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT);

    return true;
}


bool DX12_Renderer::CreateGBuffer(FrameResource& frame)
{
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    for (UINT i = 0; i < (UINT)GBufferType::Count; i++) 
    {
        const MRTTargetDesc& desc = GBUFFER_CONFIG[i];

        auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(desc.format, mWidth, mHeight, 1, 1);
        texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = desc.format;
        memcpy(clearValue.Color, desc.clearColor, sizeof(FLOAT) * 4);

        if (FAILED(mDevice->CreateCommittedResource(&heapProps,
            D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COMMON, 
            &clearValue, IID_PPV_ARGS(frame.gbuffer.targets[i].ReleaseAndGetAddressOf()))))
        {
            return false;
        }

        frame.StateTracker.Register(frame.gbuffer.targets[i].Get(), D3D12_RESOURCE_STATE_COMMON);
    }

    if (!CreateGBufferRTVs(frame)) return false;
    if (!CreateGBufferSRVs(frame)) return false;


    return true;
}

bool DX12_Renderer::CreateGBufferRTVs(FrameResource& fr)
{
    
    fr.GBufferRtvSlot_IDs.clear();
    fr.GBufferRtvSlot_IDs.reserve((UINT)GBufferType::Count);

    for (UINT g = 0; g < (UINT)GBufferType::Count; g++)
    {
        UINT slot = mRtvManager->Allocate(HeapRegion::RTV);
        fr.GBufferRtvSlot_IDs.push_back(slot);

        mDevice->CreateRenderTargetView(fr.gbuffer.targets[g].Get(), nullptr, mRtvManager->GetCpuHandle(slot));

        fr.StateTracker.Register(fr.gbuffer.targets[g].Get(), D3D12_RESOURCE_STATE_COMMON);

    }
    return true;
}

bool DX12_Renderer::CreateGBufferSRVs(FrameResource& fr)
{
    fr.GBufferSrvSlot_IDs.clear();
    fr.GBufferSrvSlot_IDs.reserve((UINT)GBufferType::Count);

    for (UINT g = 0; g < (UINT)GBufferType::Count; ++g)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Texture2D.MostDetailedMip = 0;
        srv.Texture2D.MipLevels = 1;
        srv.Format = GBUFFER_CONFIG[g].format;

        UINT slot = mResource_Heap_Manager->Allocate(HeapRegion::SRV_Frame);
        fr.GBufferSrvSlot_IDs.push_back(slot);

        auto cpu = mResource_Heap_Manager->GetCpuHandle(slot);
        mDevice->CreateShaderResourceView(fr.gbuffer.targets[g].Get(), &srv, cpu);
    }



    fr.DepthBufferSrvSlot_ID = UINT_MAX;

    if (fr.DepthStencilBuffer)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC dSrv = {};
        dSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        dSrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        dSrv.Texture2D.MostDetailedMip = 0;
        dSrv.Texture2D.MipLevels = 1;
        dSrv.Format = DXGI_FORMAT_R32_FLOAT;

        UINT dslot = mResource_Heap_Manager->Allocate(HeapRegion::SRV_Frame);
        fr.DepthBufferSrvSlot_ID = dslot;

        auto cpu = mResource_Heap_Manager->GetCpuHandle(dslot);
        mDevice->CreateShaderResourceView(fr.DepthStencilBuffer.Get(), &dSrv, cpu);
    }

    return true;
}


bool DX12_Renderer::CreateDepthStencil(FrameResource& frame, UINT width, UINT height)
{
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, width, height, 1, 1);
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // Clear value for depth/stencil
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 0.0f;
    clearValue.DepthStencil.Stencil = 0;

    if (FAILED(mDevice->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE,
        &depthDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue, IID_PPV_ARGS(frame.DepthStencilBuffer.ReleaseAndGetAddressOf()))))
    {
        return false;
    }

    frame.StateTracker.Register(frame.DepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
    return true;
}

bool DX12_Renderer::CreateDSV(FrameResource& fr)
{
    if (!CreateDepthStencil(fr, mWidth, mHeight)) return false;

    UINT slot = mDsvManager->Allocate(HeapRegion::DSV);
    fr.DsvSlot_ID = slot;

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    mDevice->CreateDepthStencilView(fr.DepthStencilBuffer.Get(), &dsvDesc, mDsvManager->GetCpuHandle(slot));

    fr.StateTracker.Register(fr.DepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

    return true;
}


bool DX12_Renderer::Create_Merge_RenderTargets(FrameResource& fr)
{
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

    for (int i = 0; i < 2; i++)
    {
        auto desc = CD3DX12_RESOURCE_DESC::Tex2D(format, mWidth, mHeight, 1, 1);
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = format;
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;

        if (FAILED(mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, 
            &desc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&fr.Merge_RenderTargets[i]))))
        {
            return false;
        }

        fr.StateTracker.Register(fr.Merge_RenderTargets[i].Get(), D3D12_RESOURCE_STATE_COMMON);


        fr.MergeRtvSlot_IDs[i] = mRtvManager->Allocate(HeapRegion::RTV);
        mDevice->CreateRenderTargetView(fr.Merge_RenderTargets[i].Get(), nullptr, mRtvManager->GetCpuHandle(fr.MergeRtvSlot_IDs[i]));

        //======================

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;

        fr.MergeSrvSlot_IDs[i] = mResource_Heap_Manager->Allocate(HeapRegion::SRV_Frame);
        mDevice->CreateShaderResourceView(fr.Merge_RenderTargets[i].Get(), &srvDesc, mResource_Heap_Manager->GetCpuHandle(fr.MergeSrvSlot_IDs[i]));
    }

    fr.Merge_Base_Index = 0; // Read(SRV)
    fr.Merge_Target_Index = 1; // Write(RTV)

    return true;
}


bool DX12_Renderer::CreateCommandList()
{
    if (FAILED(mDevice->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        mFrameResources[mFrameIndex].CommandAllocator.Get(),
        nullptr, IID_PPV_ARGS(&mCommandList))))
        return false;

    mCommandList->Close();
    return true;
}

bool DX12_Renderer::CreateFenceObjects()
{
    if (FAILED(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence))))
        return false;

    mFenceValue = 1;
    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    return (mFenceEvent != nullptr);
}

bool DX12_Renderer::CreateCommandList_Upload()
{
    if (FAILED(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mUploadAllocator))))
        return false;

    if (FAILED(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mUploadAllocator.Get(), nullptr, IID_PPV_ARGS(&mUploadCommandList))))
        return false;

    mUploadCommandList->Close();
    mUploadClosed = true;
    return true;
}

void DX12_Renderer::BeginUpload()
{
    if (mUploadClosed == false)
    {
        OutputDebugStringA("[DX12_Renderer] Warning: BeginUpload called while previous upload still open.\n");
    }

    if (mFence->GetCompletedValue() < mUploadFenceValue)
    {
        mFence->SetEventOnCompletion(mUploadFenceValue, mFenceEvent);
        WaitForSingleObject(mFenceEvent, INFINITE);
    }

    mUploadAllocator->Reset();
    mUploadCommandList->Reset(mUploadAllocator.Get(), nullptr);
    mUploadClosed = false;
}

void DX12_Renderer::EndUpload()
{
    mUploadCommandList->Close();

    ID3D12CommandList* lists[] = { mUploadCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, lists);

    mUploadFenceValue = ++mFenceValue;
    mCommandQueue->Signal(mFence.Get(), mUploadFenceValue);

    mFence->SetEventOnCompletion(mUploadFenceValue, mFenceEvent);
    WaitForSingleObject(mFenceEvent, INFINITE);

    mUploadClosed = true;
}


bool DX12_Renderer::Create_Shader()
{
    PSO_Manager& pso_manager = PSO_Manager::Instance();
    pso_manager.Init(mDevice);

    ShaderSetting geometry_ss;
    geometry_ss.vs.file = L"Shaders/Geometry_Shader.hlsl";
    geometry_ss.vs.entry = "Default_VS";
    geometry_ss.vs.target = "vs_5_1";

    geometry_ss.ps.file = L"Shaders/Geometry_Shader.hlsl";
    geometry_ss.ps.entry = "Default_PS";
    geometry_ss.ps.target = "ps_5_1";

    PipelinePreset geometry_pp;
    geometry_pp.inputlayout = InputLayoutPreset::Default;
    geometry_pp.rasterizer = RasterizerPreset::Default;
    geometry_pp.blend = BlendPreset::AlphaBlend;
    geometry_pp.depth = DepthPreset::Default;
    geometry_pp.RenderTarget = RenderTargetPreset::MRT;

    std::vector<VariantConfig> geometry_configs =
    {
        { ShaderVariant::Default, geometry_ss, geometry_pp },
        { ShaderVariant::Shadow, geometry_ss, geometry_pp }
    };

    auto geometry_shader = pso_manager.RegisterShader(
        "Geometry",
        RootSignature_Type::Default,
        geometry_configs,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
    );

    //==================================

    //ShaderSetting skinning_ss;
    //skinning_ss.cs.file = L"Shaders/Skinning_Shader.hlsl";
    //skinning_ss.cs.entry = "Skinning_CS";
    //skinning_ss.cs.target = "cs_5_1";

    //PipelinePreset skinning_pp{};

    //std::vector<VariantConfig> skinning_configs =
    //{
    //    { ShaderVariant::Skinning, skinning_ss, skinning_pp },
    //};

    //auto skinning_shader = pso_manager.RegisterComputeShader(
    //    "Skinning_Pass",
    //    RootSignature_Type::Skinning,
    //    skinning_configs
    //);

    //==================================

    ShaderSetting shadow_ss;
    shadow_ss.vs.file = L"Shaders/Shadow_Shader.hlsl";
    shadow_ss.vs.entry = "Shadow_VS";
    shadow_ss.vs.target = "vs_5_1";

    PipelinePreset shadow_pp;
    shadow_pp.inputlayout = InputLayoutPreset::Default;
    shadow_pp.rasterizer = RasterizerPreset::Shadow;
    shadow_pp.blend = BlendPreset::Opaque;
    shadow_pp.depth = DepthPreset::Default;
    shadow_pp.RenderTarget = RenderTargetPreset::ShadowMap; 

    std::vector<VariantConfig> shadow_configs =
    {
        { ShaderVariant::Shadow, shadow_ss, shadow_pp },
    };

    auto shadow_shader = pso_manager.RegisterShader(
        "ShadowMap_Pass",
        RootSignature_Type::ShadowPass,
        shadow_configs,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
    );
    
    //==================================

    ShaderSetting light_cluster_clear_ss;
    light_cluster_clear_ss.cs.file = L"Shaders/LightAssign_Shader.hlsl";
    light_cluster_clear_ss.cs.entry = "LightClusterClearCS";
    light_cluster_clear_ss.cs.target = "cs_5_1";

    ShaderSetting camera_cluster_ss;
    camera_cluster_ss.cs.file = L"Shaders/LightAssign_Shader.hlsl";
    camera_cluster_ss.cs.entry = "ClusterBoundsBuildCS";
    camera_cluster_ss.cs.target = "cs_5_1";

    ShaderSetting light_assign_ss;
    light_assign_ss.cs.file = L"Shaders/LightAssign_Shader.hlsl";
    light_assign_ss.cs.entry = "LightAssignCS";
    light_assign_ss.cs.target = "cs_5_1";

    PipelinePreset light_pp{};

    std::vector<VariantConfig> light_configs =
    {
        { ShaderVariant::LightClusterClear, light_cluster_clear_ss, light_pp },
        { ShaderVariant::ClusterBuild, camera_cluster_ss, light_pp },
        { ShaderVariant::LightAssign, light_assign_ss, light_pp },
    };

    auto light_shader = pso_manager.RegisterComputeShader(
        "Light_Pass",
        RootSignature_Type::LightPass,
        light_configs
    );

    //==================================

    ShaderSetting composite_ss;
    composite_ss.vs.file = L"Shaders/Composite_Shader.hlsl";
    composite_ss.vs.entry = "Default_VS";
    composite_ss.vs.target = "vs_5_1";

    composite_ss.ps.file = L"Shaders/Composite_Shader.hlsl";
    composite_ss.ps.entry = "Default_PS";
    composite_ss.ps.target = "ps_5_1";

    PipelinePreset composite_pp;
    composite_pp.inputlayout = InputLayoutPreset::None;
    composite_pp.rasterizer = RasterizerPreset::Default;
    composite_pp.blend = BlendPreset::AlphaBlend;
    composite_pp.depth = DepthPreset::Disabled;
    composite_pp.RenderTarget = RenderTargetPreset::OnePass;

    std::vector<VariantConfig> composite_configs =
    {
        { ShaderVariant::Default, composite_ss, composite_pp },
    };

    auto composite_shader = pso_manager.RegisterShader(
        "Composite",
        RootSignature_Type::PostFX,
        composite_configs,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
    );

    //==================================

    ShaderSetting postprocess_ss;
    postprocess_ss.vs.file = L"Shaders/PostProcess_Shader.hlsl";
    postprocess_ss.vs.entry = "Default_VS";
    postprocess_ss.vs.target = "vs_5_1";

    postprocess_ss.ps.file = L"Shaders/PostProcess_Shader.hlsl";
    postprocess_ss.ps.entry = "Default_PS";
    postprocess_ss.ps.target = "ps_5_1";

    PipelinePreset postprocess_pp;
    postprocess_pp.inputlayout = InputLayoutPreset::None;
    postprocess_pp.rasterizer = RasterizerPreset::Default;
    postprocess_pp.blend = BlendPreset::AlphaBlend;
    postprocess_pp.depth = DepthPreset::Disabled;
    postprocess_pp.RenderTarget = RenderTargetPreset::OnePass;

    std::vector<VariantConfig> postprocess_configs =
    {
        { ShaderVariant::Default, postprocess_ss, postprocess_pp },
    };

    auto postprocess_shader = pso_manager.RegisterShader(
        "PostProcess",
        RootSignature_Type::PostFX,
        postprocess_configs,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
    );

    //==================================

    ShaderSetting blit_ss;
    blit_ss.vs.file = L"Shaders/Blit_Shader.hlsl";
    blit_ss.vs.entry = "Default_VS";
    blit_ss.vs.target = "vs_5_1";

    blit_ss.ps.file = L"Shaders/Blit_Shader.hlsl";
    blit_ss.ps.entry = "Default_PS";
    blit_ss.ps.target = "ps_5_1";

    PipelinePreset blit_pp;
    blit_pp.inputlayout = InputLayoutPreset::None;
    blit_pp.rasterizer = RasterizerPreset::Default;
    blit_pp.blend = BlendPreset::AlphaBlend;
    blit_pp.depth = DepthPreset::Disabled;
    blit_pp.RenderTarget = RenderTargetPreset::OnePass;

    std::vector<VariantConfig> blit_configs =
    {
        { ShaderVariant::Default, blit_ss, blit_pp },
    };

    auto blit_shader = pso_manager.RegisterShader(
        "Blit",
        RootSignature_Type::PostFX,
        blit_configs,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
    );

    return true;
}

bool DX12_Renderer::Create_SceneCBV()
{
    UINT bufferSize = (sizeof(SceneData) + 255) & ~255;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    HRESULT hr = mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mSceneData_CB));

    hr = mSceneData_CB->Map(0, nullptr, reinterpret_cast<void**>(&mappedSceneDataCB));

    if (FAILED(hr))
        return false;

    return true;
}

bool DX12_Renderer::CreateObjectCB(FrameResource& fr, UINT maxObjects)
{
    fr.ObjectCB.MaxObjects = maxObjects;
    size_t bufferSize = maxObjects * sizeof(ObjectCBData);

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    HRESULT hr = mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&fr.ObjectCB.Buffer));

    if (FAILED(hr))
        throw std::runtime_error("Failed to create ObjectCB buffer");

    hr = fr.ObjectCB.Buffer->Map(0, nullptr, reinterpret_cast<void**>(&fr.ObjectCB.MappedObjectCB));

    if (FAILED(hr))
        throw std::runtime_error("Failed to map ObjectCB buffer");

    fr.ObjectCB.HeadOffset = 0;

    return true;
}

bool DX12_Renderer::Create_LightResources(FrameResource& fr, UINT maxLights)
{
    const RendererContext ctx = Get_UploadContext();


    //---------------------------------------
    // Cluster Buffer (UAV + SRV)
    //---------------------------------------
    {
        ComPtr<ID3D12Resource> ClusterBuffer;

        const UINT clusterBufferSize = sizeof(ClusterBound) * TOTAL_CLUSTER_COUNT;

        ClusterBuffer = ResourceUtils::CreateBufferResourceEmpty(
            ctx, clusterBufferSize,
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COMMON
        );

        if (!ClusterBuffer) return false;

        UINT clusterSRVIndex = mResource_Heap_Manager->Allocate(HeapRegion::SRV_Frame);
        UINT clusterUAVIndex = mResource_Heap_Manager->Allocate(HeapRegion::UAV);

        D3D12_SHADER_RESOURCE_VIEW_DESC clusterSRVDesc = {};
        clusterSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
        clusterSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        clusterSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        clusterSRVDesc.Buffer.FirstElement = 0;
        clusterSRVDesc.Buffer.NumElements = TOTAL_CLUSTER_COUNT;
        clusterSRVDesc.Buffer.StructureByteStride = sizeof(ClusterBound);
        clusterSRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        mDevice->CreateShaderResourceView(
            ClusterBuffer.Get(),
            &clusterSRVDesc,
            mResource_Heap_Manager->GetCpuHandle(clusterSRVIndex)
        );

        D3D12_UNORDERED_ACCESS_VIEW_DESC clusterUAVDesc = {};
        clusterUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
        clusterUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        clusterUAVDesc.Buffer.FirstElement = 0;
        clusterUAVDesc.Buffer.NumElements = TOTAL_CLUSTER_COUNT;
        clusterUAVDesc.Buffer.StructureByteStride = sizeof(ClusterBound);
        clusterUAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

        mDevice->CreateUnorderedAccessView(
            ClusterBuffer.Get(),
            nullptr,
            &clusterUAVDesc,
            mResource_Heap_Manager->GetCpuHandle(clusterUAVIndex)
        );

        fr.light_resource.ClusterBuffer = ClusterBuffer;
        fr.light_resource.ClusterBuffer_SRV_Index = clusterSRVIndex;
        fr.light_resource.ClusterBuffer_UAV_Index = clusterUAVIndex;
    }

    //---------------------------------------
    // Light Buffer (SRV 전용)
    //---------------------------------------
    {
        const UINT lightBufferSize = sizeof(GPULight) * maxLights;

        ComPtr<ID3D12Resource> LightBuffer;

        LightBuffer = ResourceUtils::CreateBufferResourceEmpty(
            ctx, lightBufferSize,
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_RESOURCE_FLAG_NONE,
            D3D12_RESOURCE_STATE_COMMON
        );

        if (!LightBuffer) return false;

        UINT lightSRVIndex = mResource_Heap_Manager->Allocate(HeapRegion::SRV_Frame);

        D3D12_SHADER_RESOURCE_VIEW_DESC lightSRVDesc = {};
        lightSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
        lightSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        lightSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        lightSRVDesc.Buffer.FirstElement = 0;
        lightSRVDesc.Buffer.NumElements = maxLights;
        lightSRVDesc.Buffer.StructureByteStride = sizeof(GPULight);
        lightSRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        mDevice->CreateShaderResourceView(
            LightBuffer.Get(),
            &lightSRVDesc,
            mResource_Heap_Manager->GetCpuHandle(lightSRVIndex)
        );

        fr.light_resource.LightBuffer = LightBuffer;
        fr.light_resource.LightBuffer_SRV_Index = lightSRVIndex;
    
    //---------------------------------------
    // Light UploadBuffer - Mapped + Copy용
    //---------------------------------------
    
        fr.light_resource.LightUploadBuffer = ResourceUtils::CreateBufferResourceEmpty(
            ctx, lightBufferSize,
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_RESOURCE_FLAG_NONE,
            D3D12_RESOURCE_STATE_GENERIC_READ
        );

        fr.light_resource.LightUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&fr.light_resource.MappedLightUploadBuffer));

        fr.StateTracker.Register(fr.light_resource.ClusterBuffer.Get(), D3D12_RESOURCE_STATE_COMMON);
        fr.StateTracker.Register(fr.light_resource.LightBuffer.Get(), D3D12_RESOURCE_STATE_COMMON);
    }


    //---------------------------------------
    // ClusterLightMetaBuffer(UAV + SRV)
    //---------------------------------------
    {
        ComPtr<ID3D12Resource> ClusterLightMetaBuffer;

        const UINT ClusterLightMetaBufferSize = sizeof(ClusterLightMeta) * TOTAL_CLUSTER_COUNT;

        ClusterLightMetaBuffer = ResourceUtils::CreateBufferResourceEmpty(
            ctx, ClusterLightMetaBufferSize,
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COMMON
        );

        if (!ClusterLightMetaBuffer) return false;

        UINT ClusterLightMetaSRVIndex = mResource_Heap_Manager->Allocate(HeapRegion::SRV_Frame);
        UINT ClusterLightMetaUAVIndex = mResource_Heap_Manager->Allocate(HeapRegion::UAV);

        D3D12_SHADER_RESOURCE_VIEW_DESC ClusterLightMetaSRVDesc = {};
        ClusterLightMetaSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
        ClusterLightMetaSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        ClusterLightMetaSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        ClusterLightMetaSRVDesc.Buffer.FirstElement = 0;
        ClusterLightMetaSRVDesc.Buffer.NumElements = TOTAL_CLUSTER_COUNT;
        ClusterLightMetaSRVDesc.Buffer.StructureByteStride = sizeof(ClusterLightMeta);
        ClusterLightMetaSRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        mDevice->CreateShaderResourceView(
            ClusterLightMetaBuffer.Get(),
            &ClusterLightMetaSRVDesc,
            mResource_Heap_Manager->GetCpuHandle(ClusterLightMetaSRVIndex)
        );

        D3D12_UNORDERED_ACCESS_VIEW_DESC ClusterLightMetaUAVDesc = {};
        ClusterLightMetaUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
        ClusterLightMetaUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        ClusterLightMetaUAVDesc.Buffer.FirstElement = 0;
        ClusterLightMetaUAVDesc.Buffer.NumElements = TOTAL_CLUSTER_COUNT;
        ClusterLightMetaUAVDesc.Buffer.StructureByteStride = sizeof(ClusterLightMeta);
        ClusterLightMetaUAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

        mDevice->CreateUnorderedAccessView(
            ClusterLightMetaBuffer.Get(),
            nullptr,
            &ClusterLightMetaUAVDesc,
            mResource_Heap_Manager->GetCpuHandle(ClusterLightMetaUAVIndex)
        );

        fr.light_resource.ClusterLightMetaBuffer = ClusterLightMetaBuffer;
        fr.light_resource.ClusterLightMetaBuffer_SRV_Index = ClusterLightMetaSRVIndex;
        fr.light_resource.ClusterLightMetaBuffer_UAV_Index = ClusterLightMetaUAVIndex;

        fr.StateTracker.Register(fr.light_resource.ClusterLightMetaBuffer.Get(), D3D12_RESOURCE_STATE_COMMON);

    }


    //---------------------------------------
    // ClusterLightIndicesBuffer(UAV + SRV)
    //---------------------------------------
    {
        ComPtr<ID3D12Resource> ClusterLightIndicesBuffer;

        const UINT MAX_CLUSTER_INCLUDE_LIGHT = 3;
        const UINT totalIndices = TOTAL_CLUSTER_COUNT * MAX_CLUSTER_INCLUDE_LIGHT;
        const UINT ClusterLightIndicesBufferSize = sizeof(UINT) * totalIndices;

        ClusterLightIndicesBuffer = ResourceUtils::CreateBufferResourceEmpty(
            ctx, ClusterLightIndicesBufferSize,
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COMMON
        );

        if (!ClusterLightIndicesBuffer) return false;

        UINT ClusterLightIndicesBufferSRVIndex = mResource_Heap_Manager->Allocate(HeapRegion::SRV_Frame);
        UINT ClusterLightIndicesBufferUAVIndex = mResource_Heap_Manager->Allocate(HeapRegion::UAV);

        D3D12_SHADER_RESOURCE_VIEW_DESC ClusterLightIndicesBufferSRVDesc = {};
        ClusterLightIndicesBufferSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
        ClusterLightIndicesBufferSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        ClusterLightIndicesBufferSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        ClusterLightIndicesBufferSRVDesc.Buffer.FirstElement = 0;
        ClusterLightIndicesBufferSRVDesc.Buffer.NumElements = totalIndices;
        ClusterLightIndicesBufferSRVDesc.Buffer.StructureByteStride = sizeof(UINT);
        ClusterLightIndicesBufferSRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        mDevice->CreateShaderResourceView(
            ClusterLightIndicesBuffer.Get(),
            &ClusterLightIndicesBufferSRVDesc,
            mResource_Heap_Manager->GetCpuHandle(ClusterLightIndicesBufferSRVIndex)
        );

        D3D12_UNORDERED_ACCESS_VIEW_DESC ClusterLightIndicesBufferUAVDesc = {};
        ClusterLightIndicesBufferUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
        ClusterLightIndicesBufferUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        ClusterLightIndicesBufferUAVDesc.Buffer.FirstElement = 0;
        ClusterLightIndicesBufferUAVDesc.Buffer.NumElements = totalIndices;
        ClusterLightIndicesBufferUAVDesc.Buffer.StructureByteStride = sizeof(UINT);
        ClusterLightIndicesBufferUAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

        mDevice->CreateUnorderedAccessView(
            ClusterLightIndicesBuffer.Get(),
            nullptr,
            &ClusterLightIndicesBufferUAVDesc,
            mResource_Heap_Manager->GetCpuHandle(ClusterLightIndicesBufferUAVIndex)
        );

        fr.light_resource.ClusterLightIndicesBuffer = ClusterLightIndicesBuffer;
        fr.light_resource.ClusterLightIndicesBuffer_SRV_Index = ClusterLightIndicesBufferSRVIndex;
        fr.light_resource.ClusterLightIndicesBuffer_UAV_Index = ClusterLightIndicesBufferUAVIndex;

        fr.StateTracker.Register(fr.light_resource.ClusterLightIndicesBuffer.Get(), D3D12_RESOURCE_STATE_COMMON);

    }


    //---------------------------------------
    // GlobalOffsetCounterBuffer(UAV 전용)
    //---------------------------------------
    {
        ComPtr<ID3D12Resource> GlobalOffsetCounterBuffer;

        const UINT GlobalOffsetCounterBufferSize = sizeof(UINT);

        GlobalOffsetCounterBuffer = ResourceUtils::CreateBufferResourceEmpty(
            ctx, GlobalOffsetCounterBufferSize,
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COMMON
        );

        if (!GlobalOffsetCounterBuffer) return false;

        UINT GlobalOffsetCounterBufferUAVIndex = mResource_Heap_Manager->Allocate(HeapRegion::UAV);

        D3D12_UNORDERED_ACCESS_VIEW_DESC GlobalOffsetCounterBufferUAVDesc = {};
        GlobalOffsetCounterBufferUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
        GlobalOffsetCounterBufferUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        GlobalOffsetCounterBufferUAVDesc.Buffer.FirstElement = 0;
        GlobalOffsetCounterBufferUAVDesc.Buffer.NumElements = 1;
        GlobalOffsetCounterBufferUAVDesc.Buffer.StructureByteStride = sizeof(UINT);
        GlobalOffsetCounterBufferUAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

        mDevice->CreateUnorderedAccessView(
            GlobalOffsetCounterBuffer.Get(),
            nullptr,
            &GlobalOffsetCounterBufferUAVDesc,
            mResource_Heap_Manager->GetCpuHandle(GlobalOffsetCounterBufferUAVIndex)
        );

        fr.light_resource.GlobalOffsetCounterBuffer = GlobalOffsetCounterBuffer;
        fr.light_resource.GlobalOffsetCounterBuffer_UAV_Index = GlobalOffsetCounterBufferUAVIndex;

        fr.StateTracker.Register(fr.light_resource.GlobalOffsetCounterBuffer.Get(), D3D12_RESOURCE_STATE_COMMON);

    }

}

bool DX12_Renderer::Create_ShadowResources(FrameResource& fr)
{
    const RendererContext ctx = Get_UploadContext();
    LightResource& lr = fr.light_resource;

    const DXGI_FORMAT SHADOW_MAP_FORMAT = DXGI_FORMAT_R32_TYPELESS;
    const DXGI_FORMAT SHADOW_MAP_DSV_FORMAT = DXGI_FORMAT_D32_FLOAT;
    const DXGI_FORMAT SHADOW_MAP_SRV_FORMAT = DXGI_FORMAT_R32_FLOAT;
    const D3D12_RESOURCE_STATES SHADOW_MAP_DEFAULT_STATE = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    const D3D12_RESOURCE_FLAGS SHADOW_MAP_FLAGS = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    //------------------------------------------------------------------
    // 1. Directional CSM Shadow Array
    {
        UINT totalCsmSlices = MAX_SHADOW_CSM * NUM_CSM_CASCADES;

        lr.CsmShadowArray = ResourceUtils::CreateTexture2DArray(
            ctx,
            CSM_SHADOW_RESOLUTION, CSM_SHADOW_RESOLUTION,
            SHADOW_MAP_FORMAT, 
            totalCsmSlices,
            SHADOW_MAP_FLAGS,
            SHADOW_MAP_DEFAULT_STATE
        );

        fr.StateTracker.Register(lr.CsmShadowArray.Get(), SHADOW_MAP_DEFAULT_STATE);

        lr.CsmShadowArray_SRV = mResource_Heap_Manager->Allocate(HeapRegion::SRV_ShadowMap_CSM);
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = SHADOW_MAP_SRV_FORMAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2DArray.MostDetailedMip = 0;
        srvDesc.Texture2DArray.MipLevels = 1;
        srvDesc.Texture2DArray.FirstArraySlice = 0;
        srvDesc.Texture2DArray.ArraySize = totalCsmSlices;
        mDevice->CreateShaderResourceView(lr.CsmShadowArray.Get(), &srvDesc, mResource_Heap_Manager->GetCpuHandle(lr.CsmShadowArray_SRV));

        lr.CsmShadow_DSVs.resize(totalCsmSlices);

        for (UINT i = 0; i < totalCsmSlices; ++i)
        {
            lr.CsmShadow_DSVs[i] = mDsvManager->Allocate(HeapRegion::DSV);
            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = SHADOW_MAP_DSV_FORMAT;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Texture2DArray.FirstArraySlice = i;
            dsvDesc.Texture2DArray.ArraySize = 1;
            dsvDesc.Texture2DArray.MipSlice = 0;
            mDevice->CreateDepthStencilView(lr.CsmShadowArray.Get(), &dsvDesc, mDsvManager->GetCpuHandle(lr.CsmShadow_DSVs[i]));
        }
    }
    //------------------------------------------------------------------
    // 2. Point Shadow Array 
    {
        lr.PointShadowCubeArray = ResourceUtils::CreateTextureCubeArray(
            ctx,
            POINT_SHADOW_RESOLUTION, POINT_SHADOW_RESOLUTION,
            SHADOW_MAP_FORMAT, 
            MAX_SHADOW_POINT,
            SHADOW_MAP_FLAGS,
            SHADOW_MAP_DEFAULT_STATE
        );
        fr.StateTracker.Register(lr.PointShadowCubeArray.Get(), SHADOW_MAP_DEFAULT_STATE);

        lr.PointShadowCubeArray_SRV = mResource_Heap_Manager->Allocate(HeapRegion::SRV_ShadowMap_Point);
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = SHADOW_MAP_SRV_FORMAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.TextureCubeArray.MostDetailedMip = 0;
        srvDesc.TextureCubeArray.MipLevels = 1;
        srvDesc.TextureCubeArray.First2DArrayFace = 0;
        srvDesc.TextureCubeArray.NumCubes = MAX_SHADOW_POINT;
        mDevice->CreateShaderResourceView(lr.PointShadowCubeArray.Get(), &srvDesc, mResource_Heap_Manager->GetCpuHandle(lr.PointShadowCubeArray_SRV));

        lr.PointShadow_DSVs.resize(MAX_SHADOW_POINT * 6);
        for (UINT cubeIndex = 0; cubeIndex < MAX_SHADOW_POINT; ++cubeIndex)
        {
            for (UINT faceIndex = 0; faceIndex < 6; ++faceIndex)
            {
                UINT dsvLinearIndex = cubeIndex * 6 + faceIndex;
                lr.PointShadow_DSVs[dsvLinearIndex] = mDsvManager->Allocate(HeapRegion::DSV);

                D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
                dsvDesc.Format = SHADOW_MAP_DSV_FORMAT;
                dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                dsvDesc.Texture2DArray.FirstArraySlice = dsvLinearIndex;
                dsvDesc.Texture2DArray.ArraySize = 1;
                dsvDesc.Texture2DArray.MipSlice = 0;
                mDevice->CreateDepthStencilView(lr.PointShadowCubeArray.Get(), &dsvDesc, mDsvManager->GetCpuHandle(lr.PointShadow_DSVs[dsvLinearIndex]));
            }
        }
    }
    //------------------------------------------------------------------
    // 3. Spot Shadow Array 
    {
        lr.SpotShadowArray = ResourceUtils::CreateTexture2DArray(
            ctx,
            SPOT_SHADOW_RESOLUTION, SPOT_SHADOW_RESOLUTION,
            SHADOW_MAP_FORMAT, 
            MAX_SHADOW_SPOT,
            SHADOW_MAP_FLAGS,
            SHADOW_MAP_DEFAULT_STATE
        );
        fr.StateTracker.Register(lr.SpotShadowArray.Get(), SHADOW_MAP_DEFAULT_STATE);

        lr.SpotShadowArray_SRV = mResource_Heap_Manager->Allocate(HeapRegion::SRV_ShadowMap_Spot);
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = SHADOW_MAP_SRV_FORMAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2DArray.MostDetailedMip = 0;
        srvDesc.Texture2DArray.MipLevels = 1;
        srvDesc.Texture2DArray.FirstArraySlice = 0;
        srvDesc.Texture2DArray.ArraySize = MAX_SHADOW_SPOT;
        mDevice->CreateShaderResourceView(lr.SpotShadowArray.Get(), &srvDesc, mResource_Heap_Manager->GetCpuHandle(lr.SpotShadowArray_SRV));

        lr.SpotShadow_DSVs.resize(MAX_SHADOW_SPOT);
        for (UINT i = 0; i < MAX_SHADOW_SPOT; ++i)
        {
            lr.SpotShadow_DSVs[i] = mDsvManager->Allocate(HeapRegion::DSV);
            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = SHADOW_MAP_DSV_FORMAT;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Texture2DArray.FirstArraySlice = i;
            dsvDesc.Texture2DArray.ArraySize = 1;
            dsvDesc.Texture2DArray.MipSlice = 0;
            mDevice->CreateDepthStencilView(lr.SpotShadowArray.Get(), &dsvDesc, mDsvManager->GetCpuHandle(lr.SpotShadow_DSVs[i]));
        }
    }


    return true;
}

bool DX12_Renderer::CreateShadowMatrixBuffer(FrameResource& fr)
{
    LightResource& lr = fr.light_resource;

    UINT bufferSize = (sizeof(ShadowMatrixData) * MAX_SHADOW_VIEWS);
    bufferSize = (bufferSize + 255) & ~255;

    auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    HRESULT hr = mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&lr.ShadowMatrixBuffer));

    if (FAILED(hr))
    {
        OutputDebugStringA("Failed to create ShadowMatrixBuffer.\n");
        return false;
    }

    hr = lr.ShadowMatrixBuffer->Map(0, nullptr, reinterpret_cast<void**>(&lr.MappedShadowMatrixBuffer));
    if (FAILED(hr))
    {
        OutputDebugStringA("Failed to map ShadowMatrixBuffer.\n");
        return false;
    }

    lr.ShadowMatrixBuffer_SRV_Index = mResource_Heap_Manager->Allocate(HeapRegion::SRV_Frame);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = 0;

    srvDesc.Buffer.NumElements = MAX_SHADOW_VIEWS;
    srvDesc.Buffer.StructureByteStride = sizeof(ShadowMatrixData);
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    mDevice->CreateShaderResourceView(lr.ShadowMatrixBuffer.Get(), &srvDesc, 
        mResource_Heap_Manager->GetCpuHandle(lr.ShadowMatrixBuffer_SRV_Index));

    return true;
}

// ------------------- Rendering Steps -------------------

FrameResource& DX12_Renderer::GetCurrentFrameResource()
{
    FrameResource& fr = mFrameResources[mFrameIndex];

    // GPU가 해당 FrameResource를 아직 사용 중이라면 기다림
    if (mFence->GetCompletedValue() < mFrameFenceValues[mFrameIndex])
    {
        mFence->SetEventOnCompletion(mFrameFenceValues[mFrameIndex], mFenceEvent);
        WaitForSingleObject(mFenceEvent, INFINITE);
    }

    mRtvManager->Update();
    mDsvManager->Update();
    mResource_Heap_Manager->Update();
    return fr;
}

void DX12_Renderer::Update_SceneCBV(const SceneData& data)
{
    memcpy(mappedSceneDataCB, &data, sizeof(SceneData));
}

void DX12_Renderer::Bind_SceneCBV(Shader_Type shader_type, UINT rootParameter)
{
    if(shader_type == Shader_Type::Graphics)
        mCommandList->SetGraphicsRootConstantBufferView(rootParameter, mSceneData_CB->GetGPUVirtualAddress());
	else if (shader_type == Shader_Type::Compute)
		mCommandList->SetComputeRootConstantBufferView(rootParameter, mSceneData_CB->GetGPUVirtualAddress());
}

void DX12_Renderer::UpdateObjectCBs(const std::vector<RenderData>& renderables)
{
    FrameResource& fr = mFrameResources[mFrameIndex];
    fr.ObjectCB.HeadOffset = 0;
    mDrawItems.clear();

    ResourceSystem* rsm = GameEngine::Get().GetResourceSystem();
    Material defaultMaterial = Material::Get_Default();

    for (auto& rd : renderables)
    {
        auto transform = rd.transform.lock();
        auto renderer = rd.meshRenderer.lock();
        if (!transform || !renderer) continue;

        auto skinnedComp = std::dynamic_pointer_cast<SkinnedMeshRendererComponent>(renderer);

        auto mesh = renderer->GetMesh();
        if (!mesh) continue;

        XMFLOAT4X4 worldT = Matrix4x4::Transpose(transform->GetWorldMatrix());

        const size_t submeshCount = mesh->submeshes.size();
        for (size_t i = 0; i < submeshCount; ++i)
        {
            UINT matId = renderer->GetMaterial(i);
            if (matId == Engine::INVALID_ID)
                matId = mesh->submeshes[i].materialId;
            auto material = rsm->GetById<Material>(matId);
            const Material* matToUse = material ? material.get() : &defaultMaterial;

            ObjectCBData cb{};
            cb.World = worldT;
            cb.Albedo = XMFLOAT4(matToUse->albedoColor.x, matToUse->albedoColor.y, matToUse->albedoColor.z, 1.0f);
            cb.Roughness = matToUse->roughness;
            cb.Metallic = matToUse->metallic;
            cb.Emissive = 0.0f;

            auto toIdx = [](UINT slot)->int { return (slot == UINT_MAX) ? -1 : static_cast<int>(slot); };
            cb.DiffuseTexIdx = toIdx(matToUse->diffuseTexSlot);
            cb.NormalTexIdx = toIdx(matToUse->normalTexSlot);
            cb.RoughnessTexIdx = toIdx(matToUse->roughnessTexSlot);
            cb.MetallicTexIdx = toIdx(matToUse->metallicTexSlot);

            const UINT cbIndex = fr.ObjectCB.HeadOffset++;
            fr.ObjectCB.MappedObjectCB[cbIndex] = cb;

            DrawItem di{};
            di.mesh = mesh.get();
            di.sub = mesh->submeshes[i];
            di.cbIndex = cbIndex;
            di.materialId = (material) ? matId : Engine::INVALID_ID;

            if (skinnedComp)
                di.skinnedComp = skinnedComp.get();

            mDrawItems.emplace_back(std::move(di));
        }
    }
}

void DX12_Renderer::UpdateLightAndShadowData(std::shared_ptr<CameraComponent> render_camera, const std::vector<LightComponent*>& light_comp_list)
{
    FrameResource& fr = mFrameResources[mFrameIndex];
    LightResource& lr = fr.light_resource;
    const UINT currentFrameIndex = mFrameIndex;

    lr.mFrameShadowCastingCSM.clear();
    lr.mFrameShadowCastingSpot.clear();
    lr.mFrameShadowCastingPoint.clear();
    lr.mLightShadowIndexMap.clear();

    UINT csmShadowCount = 0;
    UINT spotShadowCount = 0;
    UINT pointShadowCount = 0;

    const UINT pointBaseOffset = 0;
    const UINT csmBaseOffset = pointBaseOffset + (MAX_SHADOW_POINT * 6);
    const UINT spotBaseOffset = csmBaseOffset + (MAX_SHADOW_CSM * NUM_CSM_CASCADES);

    if (render_camera->IsViewMatrixUpdatedThisFrame())
        for (const auto& light : light_comp_list) light->NotifyCameraMoved();

    XMMATRIX view_matrix = render_camera->GetViewMatrix();
    std::vector<GPULight> view_space_lights;
    view_space_lights.reserve(light_comp_list.size());

    for (const auto& world_light : light_comp_list)
    {
        // [수정] ToGPUData() 호출을 루프 후반부로 이동시켰습니다.

        XMVECTOR world_pos = XMLoadFloat3(&world_light->GetPosition());
        XMVECTOR world_dir = XMLoadFloat3(&world_light->GetDirection());

        // shadowMapStartIndex와 shadowMapLength를 임시 저장할 변수
        UINT shadowBaseIndex = Engine::INVALID_ID;
        UINT shadowMatrixCount = 0;

        if (world_light->CastsShadow())
        {
            Light_Type type = world_light->GetLightType();
            UINT baseIndex = 0, matrixCount = 0;
            bool assigned = false;

            if (type == Light_Type::Point && pointShadowCount < MAX_SHADOW_POINT)
            {
                baseIndex = pointBaseOffset + (pointShadowCount * 6);
                matrixCount = 6;
                pointShadowCount++;
                assigned = true;

                if (world_light->NeedsShadowUpdate(currentFrameIndex))
                {
                    lr.mFrameShadowCastingPoint.push_back(world_light);
                    for (UINT i = 0; i < 6; ++i)
                        lr.MappedShadowMatrixBuffer[baseIndex + i].ViewProj = world_light->UpdateShadowViewProj(nullptr, i);
                    world_light->ClearStaticBakeFlag(currentFrameIndex);
                }
            }
            else if (type == Light_Type::Directional && csmShadowCount < MAX_SHADOW_CSM)
            {
                baseIndex = csmBaseOffset + (csmShadowCount * NUM_CSM_CASCADES);
                matrixCount = NUM_CSM_CASCADES;
                csmShadowCount++;
                assigned = true;

                if (world_light->NeedsShadowUpdate(currentFrameIndex))
                {
                    lr.mFrameShadowCastingCSM.push_back(world_light);
                    // 이 UpdateShadowViewProj 호출이 C++ 멤버(cascadeSplits)를 먼저 업데이트합니다.
                    if (world_light->GetDirectionalShadowMode() == DirectionalShadowMode::CSM)
                        for (UINT i = 0; i < NUM_CSM_CASCADES; ++i)
                            lr.MappedShadowMatrixBuffer[baseIndex + i].ViewProj =
                            world_light->UpdateShadowViewProj(render_camera, i);
                    else
                        lr.MappedShadowMatrixBuffer[baseIndex].ViewProj =
                        world_light->UpdateShadowViewProj(nullptr, 0);
                    world_light->ClearStaticBakeFlag(currentFrameIndex);
                }
            }
            else if (type == Light_Type::Spot && spotShadowCount < MAX_SHADOW_SPOT)
            {
                baseIndex = spotBaseOffset + spotShadowCount;
                matrixCount = 1;
                spotShadowCount++;
                assigned = true;

                if (world_light->NeedsShadowUpdate(currentFrameIndex))
                {
                    lr.mFrameShadowCastingSpot.push_back(world_light);
                    lr.MappedShadowMatrixBuffer[baseIndex].ViewProj = world_light->UpdateShadowViewProj(nullptr, 0);
                    world_light->ClearStaticBakeFlag(currentFrameIndex);
                }
            }

            if (assigned)
            {
                lr.mLightShadowIndexMap[world_light] = baseIndex;
                shadowBaseIndex = baseIndex;   // 임시 변수에 저장
                shadowMatrixCount = matrixCount; // 임시 변수에 저장
            }
        }

        // [수정] UpdateShadowViewProj()가 호출된 이후에 ToGPUData()를 호출합니다.
        // 이렇게 하면 cascadeSplits 멤버가 최신 값으로 채워진 GPULight 구조체가 생성됩니다.
        GPULight view_light = world_light->ToGPUData();

        // View Space 변환 적용
        XMStoreFloat3(&view_light.position, XMVector3TransformCoord(world_pos, view_matrix));
        XMStoreFloat3(&view_light.direction, XMVector3Normalize(XMVector3TransformNormal(world_dir, view_matrix)));

        // 임시 저장했던 섀도우 인덱스 정보를 최종 GPU 데이터에 할당
        view_light.shadowMapStartIndex = shadowBaseIndex;
        view_light.shadowMapLength = shadowMatrixCount;

        view_space_lights.push_back(view_light);
    }

    memcpy(lr.MappedLightUploadBuffer, view_space_lights.data(), sizeof(GPULight) * view_space_lights.size());

    fr.StateTracker.Transition(mCommandList.Get(), lr.LightBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
    mCommandList->CopyBufferRegion(lr.LightBuffer.Get(), 0, lr.LightUploadBuffer.Get(), 0, sizeof(GPULight) * view_space_lights.size());
    fr.StateTracker.Transition(mCommandList.Get(), lr.LightBuffer.Get(), D3D12_RESOURCE_STATE_COMMON);
}

void DX12_Renderer::CullObjectsForShadow(LightComponent* light, UINT cascadeIdx)
{
    mVisibleItems.clear();

    FrameResource& fr = mFrameResources[mFrameIndex];
    ObjectCBData* objCBArray = fr.ObjectCB.MappedObjectCB;

    const Light_Type lightType = light->GetLightType();
    const BoundingBox clipAABB(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1));

    if (lightType == Light_Type::Directional)
    {
        const XMFLOAT4X4& cachedTP = light->GetShadowViewProj(cascadeIdx);
        XMMATRIX lightViewProjT = XMLoadFloat4x4(&cachedTP);
        XMMATRIX lightViewProj = XMMatrixTranspose(lightViewProjT);

        for (const auto& di : mDrawItems)
        {
            if (!di.mesh) continue;

            const ObjectCBData& objData = objCBArray[di.cbIndex];
            XMMATRIX world = XMMatrixTranspose(XMLoadFloat4x4(&objData.World));

            BoundingBox worldAABB;
            di.sub.localAABB.Transform(worldAABB, world);

            BoundingBox lightAABB;
            worldAABB.Transform(lightAABB, lightViewProj);

            if (lightAABB.Intersects(clipAABB))
                mVisibleItems.push_back(di);
        }
    }
    else if (lightType == Light_Type::Point)
    {
        const XMFLOAT3 lightPos = light->GetPosition();
        float r = std::min(light->GetRange(), light->GetShadowMapFar());
        BoundingSphere lightSphere(lightPos, r);

        for (const auto& di : mDrawItems)
        {
            if (!di.mesh) continue;

            const ObjectCBData& objData = objCBArray[di.cbIndex];
            XMMATRIX world = XMMatrixTranspose(XMLoadFloat4x4(&objData.World));

            BoundingBox worldAABB;
            di.sub.localAABB.Transform(worldAABB, world);

            if (worldAABB.Intersects(lightSphere))
                mVisibleItems.push_back(di);
        }
    }
    else if (lightType == Light_Type::Spot)
    {
        const XMFLOAT3 pos = light->GetPosition();
        const XMFLOAT3 dir = light->GetDirection();
        const float nearZ = light->GetShadowMapNear();
        const float farZ = light->GetShadowMapFar();
        const float fov = light->GetOuterAngle();

        XMVECTOR eye = XMLoadFloat3(&pos);
        XMVECTOR look = XMVector3Normalize(XMLoadFloat3(&dir));
        XMVECTOR up = (fabsf(XMVectorGetY(look)) > 0.99f) ? XMVectorSet(0, 0, 1, 0) : XMVectorSet(0, 1, 0, 0);

        XMMATRIX view = XMMatrixLookToLH(eye, look, up);
        XMMATRIX proj = XMMatrixPerspectiveFovLH(fov, 1.0f, nearZ, farZ);

        BoundingFrustum frSpot;
        BoundingFrustum::CreateFromMatrix(frSpot, proj);

        XMMATRIX invView = XMMatrixInverse(nullptr, view);
        frSpot.Transform(frSpot, invView);

        for (const auto& di : mDrawItems)
        {
            if (!di.mesh) continue;

            const ObjectCBData& objData = objCBArray[di.cbIndex];
            XMMATRIX world = XMMatrixTranspose(XMLoadFloat4x4(&objData.World));

            BoundingBox worldAABB;
            di.sub.localAABB.Transform(worldAABB, world);

            if (frSpot.Intersects(worldAABB))
                mVisibleItems.push_back(di);
        }
    }
}

void DX12_Renderer::CullObjectsForRender(std::shared_ptr<CameraComponent> camera)
{
    mVisibleItems.clear();

    const BoundingFrustum& frustum = camera->GetFrustumWS();
    FrameResource& fr = mFrameResources[mFrameIndex];
    const ObjectCBData* objCBArray = fr.ObjectCB.MappedObjectCB;

    for (const auto& di : mDrawItems)
    {
        if (!di.mesh) continue;

        const ObjectCBData& objData = objCBArray[di.cbIndex];
        XMMATRIX world = XMMatrixTranspose(XMLoadFloat4x4(&objData.World));

        BoundingBox worldAABB;
        di.sub.localAABB.Transform(worldAABB, world);

        if (frustum.Intersects(worldAABB))
            mVisibleItems.push_back(di);
    }
}

void DX12_Renderer::Render_Objects(ComPtr<ID3D12GraphicsCommandList> cmdList, UINT objectCBVRootParamIndex, const std::vector<DrawItem>& drawList)
{
    FrameResource& fr = mFrameResources[mFrameIndex];

    for (const auto& di : drawList)
    {
        if (!di.mesh) continue;

        D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = fr.ObjectCB.Buffer->GetGPUVirtualAddress() + di.cbIndex * sizeof(ObjectCBData);

        cmdList->SetGraphicsRootConstantBufferView(objectCBVRootParamIndex, gpuAddr);

        di.mesh->Bind(cmdList);
        cmdList->DrawIndexedInstanced(di.sub.indexCount, 1, di.sub.startIndexLocation, di.sub.baseVertexLocation, 0);
    }
}


void DX12_Renderer::PrepareCommandList()
{
    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    FrameResource& fr = GetCurrentFrameResource();

    fr.CommandAllocator->Reset();
    mCommandList->Reset(fr.CommandAllocator.Get(), nullptr);
}

void DX12_Renderer::ClearGBuffer()
{
    FrameResource& fr = mFrameResources[mFrameIndex];

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs;

    rtvs.reserve(fr.GBufferRtvSlot_IDs.size());

    for (UINT slot : fr.GBufferRtvSlot_IDs)
        rtvs.push_back(mRtvManager->GetCpuHandle(slot));

    for (auto& target : fr.gbuffer.targets)
        fr.StateTracker.Transition(mCommandList.Get(), target.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto dsv = mDsvManager->GetCpuHandle(fr.DsvSlot_ID);

    fr.StateTracker.Transition(mCommandList.Get(), fr.DepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

    mCommandList->OMSetRenderTargets((UINT)rtvs.size(), rtvs.data(), FALSE, &dsv);

    //===================

    for (UINT g = 0; g < (UINT)GBufferType::Count; g++)
    {
        const MRTTargetDesc& cfg = GBUFFER_CONFIG[g];
        mCommandList->ClearRenderTargetView(rtvs[g], cfg.clearColor, 0, nullptr);
    }

	mCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr); // Clear to far plane 0.0f for reversed Z

    for (auto& target : fr.gbuffer.targets)
        fr.StateTracker.Transition(mCommandList.Get(), target.Get(), D3D12_RESOURCE_STATE_COMMON);
}

void DX12_Renderer::PrepareGBuffer_RTV()
{
    FrameResource& fr = mFrameResources[mFrameIndex];

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs;

    rtvs.reserve(fr.GBufferRtvSlot_IDs.size());

    for (UINT slot : fr.GBufferRtvSlot_IDs)
        rtvs.push_back(mRtvManager->GetCpuHandle(slot));

    auto dsv = mDsvManager->GetCpuHandle(fr.DsvSlot_ID);

    for (auto& target : fr.gbuffer.targets)
        fr.StateTracker.Transition(mCommandList.Get(), target.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);

    mCommandList->OMSetRenderTargets((UINT)rtvs.size(), rtvs.data(), FALSE, &dsv);
}

void DX12_Renderer::PrepareGBuffer_SRV()
{
    FrameResource& fr = mFrameResources[mFrameIndex];

    for (auto& target : fr.gbuffer.targets)
        fr.StateTracker.Transition(mCommandList.Get(), target.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    if (fr.DepthStencilBuffer)
        fr.StateTracker.Transition(mCommandList.Get(), fr.DepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void DX12_Renderer::ClearBackBuffer(float clear_color[4])
{
    FrameResource& fr = mFrameResources[mFrameIndex];

    fr.StateTracker.Transition(mCommandList.Get(), fr.RenderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto rtv = mRtvManager->GetCpuHandle(fr.BackBufferRtvSlot_ID);

    mCommandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    mCommandList->ClearRenderTargetView(rtv, clear_color, 0, nullptr);
}

void DX12_Renderer::TransitionBackBufferToPresent()
{
    FrameResource& fr = mFrameResources[mFrameIndex];

    fr.StateTracker.Transition(mCommandList.Get(), fr.RenderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT);
}

void DX12_Renderer::PresentFrame()
{
    ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, cmdLists);

    mSwapChain->Present(0, 0); // (1, 0)  -> VSync : 모니터 주사율에 맞춤

    const UINT64 currentFence = ++mFenceValue;
    mCommandQueue->Signal(mFence.Get(), currentFence);

    mFrameFenceValues[mFrameIndex] = currentFence;
    mFrameResources[mFrameIndex].FenceValue = currentFence;
}

void DX12_Renderer::Render(std::shared_ptr<Scene> render_scene)
{
    std::shared_ptr<CameraComponent> mainCam = render_scene->GetActiveCamera();
    std::vector<RenderData> renderData_list = render_scene->GetRenderable();
    std::vector<LightComponent*> light_comp_list = render_scene->GetLightList();

    if (!mainCam)
        return;

    mainCam->Update();
    mainCam->UpdateCBV();


    //--------------------------------------------------------------------------------------
    PrepareCommandList();
    
    ID3D12DescriptorHeap* heaps[] = { mResource_Heap_Manager->GetHeap() };
    mCommandList->SetDescriptorHeaps(1, heaps);

    mainCam->SetViewportsAndScissorRects(mCommandList);

    //------------------------------------------
    UpdateObjectCBs(renderData_list);
    CullObjectsForRender(mainCam);
	SkinningPass();
    GeometryPass(mainCam);
    //------------------------------------------
    UpdateLightAndShadowData(mainCam, light_comp_list);
    LightPass(mainCam);
    ShadowPass();
	//------------------------------------------

    mainCam->SetViewportsAndScissorRects(mCommandList);

    CompositePass(mainCam);

    PostProcessPass(mainCam);

    Blit_BackBufferPass();

    ImguiPass();
    TransitionBackBufferToPresent();
    mCommandList->Close();
    PresentFrame();
}

void DX12_Renderer::SkinningPass()
{
    struct SkinningConstants
    {
        UINT vertexCount;
        UINT hotStride;
        UINT skinStride;
        UINT boneCount;
    };

    FrameResource& fr = mFrameResources[mFrameIndex];
    ID3D12RootSignature* skinning_rootsignature = RootSignatureFactory::Get(RootSignature_Type::Skinning);
    mCommandList->SetComputeRootSignature(skinning_rootsignature);
    PSO_Manager::Instance().BindShader(mCommandList, "Skinning", ShaderVariant::Skinning);

    for (const auto& di : mVisibleItems)
    {
        if (!di.skinnedComp) continue;

        FrameSkinBuffer& fsb = di.skinnedComp->GetFrameSkinBuffer(mFrameIndex);

        if (!di.skinnedComp->HasValidBuffers())
        {
            fsb.mIsSkinningResultReady = false;
            continue;
        }

        auto animController = di.skinnedComp->GetAnimController();
        if (!animController)
        {
            fsb.mIsSkinningResultReady = false;
            continue;
        }

        UINT boneSRVSlot = animController->GetBoneMatrixSRV();
        auto skeleton = animController->GetSkeleton();

        if (boneSRVSlot == UINT_MAX || !skeleton)
        {
            fsb.mIsSkinningResultReady = false;
            continue;
        }

        SkinnedMesh* skinMesh = static_cast<SkinnedMesh*>(di.mesh);

        SkinningConstants constants = {
            skinMesh->GetVertexCount(),
            skinMesh->GetHotStride(),
            sizeof(GPU_SkinData),
            (UINT)skeleton->BoneList.size()
        };

        mCommandList->SetComputeRoot32BitConstants(0, 4, &constants, 0);

        mCommandList->SetComputeRootDescriptorTable(1, mResource_Heap_Manager->GetGpuHandle(skinMesh->GetSkinDataSRV()));
        mCommandList->SetComputeRootDescriptorTable(2, mResource_Heap_Manager->GetGpuHandle(skinMesh->GetHotInputSRV()));
        mCommandList->SetComputeRootDescriptorTable(3, mResource_Heap_Manager->GetGpuHandle(boneSRVSlot));
        mCommandList->SetComputeRootDescriptorTable(4, mResource_Heap_Manager->GetGpuHandle(fsb.uavSlot));

        fr.StateTracker.Transition(mCommandList.Get(), fsb.skinnedBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        UINT threadGroupCount = (constants.vertexCount + 63) / 64;
        mCommandList->Dispatch(threadGroupCount, 1, 1);

        fr.StateTracker.UAVBarrier(mCommandList.Get(), fsb.skinnedBuffer.Get());

        fsb.mIsSkinningResultReady = true;
    }
}
void DX12_Renderer::GeometryPass(std::shared_ptr<CameraComponent> render_camera)
{
    FrameResource& fr = mFrameResources[mFrameIndex];

    ClearBackBuffer(clear_color);
    ClearGBuffer();
    PrepareGBuffer_RTV();

    ID3D12RootSignature* default_rootsignature = RootSignatureFactory::Get(RootSignature_Type::Default);
    mCommandList->SetGraphicsRootSignature(default_rootsignature);

    PSO_Manager::Instance().BindShader(mCommandList, "Geometry", ShaderVariant::Default);

    //============================================

    mCommandList->SetGraphicsRootDescriptorTable(RootParameter_Default::TextureTable, mResource_Heap_Manager->GetRegionStartHandle(HeapRegion::SRV_Texture));

    Bind_SceneCBV(Shader_Type::Graphics, RootParameter_Default::SceneCBV);

    render_camera->Graphics_Bind(mCommandList, RootParameter_Default::CameraCBV);

    Render_Objects(mCommandList, RootParameter_Default::ObjectCBV, mVisibleItems);
}

void DX12_Renderer::LightPass(std::shared_ptr<CameraComponent> render_camera)
{
    FrameResource& fr = mFrameResources[mFrameIndex];

    ID3D12RootSignature* rs = RootSignatureFactory::Get(RootSignature_Type::LightPass);
    mCommandList->SetComputeRootSignature(rs);

    //============================================
    {
        PSO_Manager::Instance().BindShader(mCommandList, "Light_Pass", ShaderVariant::LightClusterClear);

        render_camera->Compute_Bind(mCommandList, RootParameter_LightPass::CameraCBV);

        Bind_SceneCBV(Shader_Type::Compute, RootParameter_LightPass::SceneCBV);

        fr.StateTracker.Transition(mCommandList.Get(), fr.light_resource.ClusterLightMetaBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        fr.StateTracker.Transition(mCommandList.Get(), fr.light_resource.GlobalOffsetCounterBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		auto LightMetaUav = mResource_Heap_Manager->GetGpuHandle(fr.light_resource.ClusterLightMetaBuffer_UAV_Index);
		mCommandList->SetComputeRootDescriptorTable(RootParameter_LightPass::ClusterLightMetaUAV, LightMetaUav);

		auto GlobalOffsetUav = mResource_Heap_Manager->GetGpuHandle(fr.light_resource.GlobalOffsetCounterBuffer_UAV_Index);
		mCommandList->SetComputeRootDescriptorTable(RootParameter_LightPass::GlobalCounterUAV, GlobalOffsetUav);

        UINT dispatchX = (TOTAL_CLUSTER_COUNT + 63) / 64;
        mCommandList->Dispatch(dispatchX, 1, 1);

        fr.StateTracker.UAVBarrier(mCommandList.Get(), fr.light_resource.GlobalOffsetCounterBuffer.Get());
    }
    //============================================
    {
        PSO_Manager::Instance().BindShader(mCommandList, "Light_Pass", ShaderVariant::ClusterBuild);

        render_camera->Compute_Bind(mCommandList, RootParameter_LightPass::CameraCBV);

        Bind_SceneCBV(Shader_Type::Compute, RootParameter_LightPass::SceneCBV);

        fr.StateTracker.Transition(mCommandList.Get(), fr.light_resource.ClusterBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        auto ClusterUav = mResource_Heap_Manager->GetGpuHandle(fr.light_resource.ClusterBuffer_UAV_Index);
        mCommandList->SetComputeRootDescriptorTable(RootParameter_LightPass::ClusterAreaUAV, ClusterUav);

        UINT dispatchX = (CLUSTER_X + 7) / 8;
        UINT dispatchY = (CLUSTER_Y + 7) / 8;
        UINT dispatchZ = CLUSTER_Z;

        mCommandList->Dispatch(dispatchX, dispatchY, dispatchZ);

        fr.StateTracker.UAVBarrier(mCommandList.Get(), fr.light_resource.ClusterBuffer.Get());
    }
    //============================================
    {
        PSO_Manager::Instance().BindShader(mCommandList, "Light_Pass", ShaderVariant::LightAssign);

        render_camera->Compute_Bind(mCommandList, RootParameter_LightPass::CameraCBV);

        Bind_SceneCBV(Shader_Type::Compute, RootParameter_LightPass::SceneCBV);

        fr.StateTracker.Transition(mCommandList.Get(), fr.light_resource.ClusterBuffer.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        fr.StateTracker.Transition(mCommandList.Get(), fr.light_resource.ClusterLightMetaBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        fr.StateTracker.Transition(mCommandList.Get(), fr.light_resource.ClusterLightIndicesBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        auto ClusterSrv = mResource_Heap_Manager->GetGpuHandle(fr.light_resource.ClusterBuffer_SRV_Index);
        mCommandList->SetComputeRootDescriptorTable(RootParameter_LightPass::ClusterAreaSRV, ClusterSrv);

        auto LightSrv = mResource_Heap_Manager->GetGpuHandle(fr.light_resource.LightBuffer_SRV_Index);
        mCommandList->SetComputeRootDescriptorTable(RootParameter_LightPass::LightBufferSRV, LightSrv);

        auto LightMetaUav = mResource_Heap_Manager->GetGpuHandle(fr.light_resource.ClusterLightMetaBuffer_UAV_Index);
        mCommandList->SetComputeRootDescriptorTable(RootParameter_LightPass::ClusterLightMetaUAV, LightMetaUav);

		auto LightIndicesUav = mResource_Heap_Manager->GetGpuHandle(fr.light_resource.ClusterLightIndicesBuffer_UAV_Index);
		mCommandList->SetComputeRootDescriptorTable(RootParameter_LightPass::ClusterLightIndicesUAV, LightIndicesUav);

        auto GlobalOffsetUav = mResource_Heap_Manager->GetGpuHandle(fr.light_resource.GlobalOffsetCounterBuffer_UAV_Index);
        mCommandList->SetComputeRootDescriptorTable(RootParameter_LightPass::GlobalCounterUAV, GlobalOffsetUav);


        UINT dispatchX = (TOTAL_CLUSTER_COUNT + 63) / 64;
        mCommandList->Dispatch(dispatchX, 1, 1);

        fr.StateTracker.UAVBarrier(mCommandList.Get(), fr.light_resource.ClusterLightIndicesBuffer.Get());
        fr.StateTracker.UAVBarrier(mCommandList.Get(), fr.light_resource.ClusterLightMetaBuffer.Get());
    }
    //============================================
}

void DX12_Renderer::ShadowPass()
{
    FrameResource& fr = GetCurrentFrameResource();
    LightResource& lr = fr.light_resource;

    ID3D12RootSignature* shadow_rs = RootSignatureFactory::Get(RootSignature_Type::ShadowPass);
    mCommandList->SetGraphicsRootSignature(shadow_rs);

    PSO_Manager::Instance().BindShader(mCommandList, "ShadowMap_Pass", ShaderVariant::Shadow);

    mCommandList->SetGraphicsRootDescriptorTable(RootParameter_Shadow::ShadowMatrix_SRV, mResource_Heap_Manager->GetGpuHandle(lr.ShadowMatrixBuffer_SRV_Index));

    //============================================
    // A. Point (Cube)
    D3D12_VIEWPORT pointViewport = LightComponent::Get_ShadowMapViewport(Light_Type::Point);
    D3D12_RECT pointScissor = LightComponent::Get_ShadowMapScissorRect(Light_Type::Point);

    mCommandList->RSSetViewports(1, &pointViewport);
    mCommandList->RSSetScissorRects(1, &pointScissor);

    fr.StateTracker.Transition(mCommandList.Get(), lr.PointShadowCubeArray.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

    for (UINT i = 0; i < lr.mFrameShadowCastingPoint.size(); ++i)
    {
        LightComponent* light = lr.mFrameShadowCastingPoint[i];
        UINT baseMatrixIndex = lr.mLightShadowIndexMap[light];
        UINT baseDsvIndex = i * 6;

        for (UINT faceIndex = 0; faceIndex < 6; ++faceIndex)
        {
            UINT matrixIndex = baseMatrixIndex + faceIndex;
            UINT dsvIndex = baseDsvIndex + faceIndex;

            auto dsv = mDsvManager->GetCpuHandle(lr.PointShadow_DSVs[dsvIndex]);
            mCommandList->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
            mCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr); // Clear to far plane 0.0f for reversed Z

            mCommandList->SetGraphicsRoot32BitConstants(RootParameter_Shadow::ShadowMatrix_Index, 1, &matrixIndex, 0);

            CullObjectsForShadow(light, 0);
            Render_Objects(mCommandList, RootParameter_Shadow::ObjectCBV, mVisibleItems);
        }
    }

    fr.StateTracker.Transition(mCommandList.Get(), lr.PointShadowCubeArray.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // =======================================================
    // B. Directional (CSM)
    D3D12_VIEWPORT csmViewport = LightComponent::Get_ShadowMapViewport(Light_Type::Directional);
    D3D12_RECT csmScissor = LightComponent::Get_ShadowMapScissorRect(Light_Type::Directional);

    mCommandList->RSSetViewports(1, &csmViewport);
    mCommandList->RSSetScissorRects(1, &csmScissor);

    fr.StateTracker.Transition(mCommandList.Get(), lr.CsmShadowArray.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

    for (UINT i = 0; i < lr.mFrameShadowCastingCSM.size(); ++i)
    {
        LightComponent* light = lr.mFrameShadowCastingCSM[i];
        UINT baseMatrixIndex = lr.mLightShadowIndexMap[light];
        UINT baseDsvIndex = i * NUM_CSM_CASCADES;

        for (UINT cascadeIndex = 0; cascadeIndex < NUM_CSM_CASCADES; ++cascadeIndex)
        {
            UINT matrixIndex = baseMatrixIndex + cascadeIndex;
            UINT dsvIndex = baseDsvIndex + cascadeIndex;

            auto dsv = mDsvManager->GetCpuHandle(lr.CsmShadow_DSVs[dsvIndex]);
            mCommandList->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
            mCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr); // Clear to far plane 0.0f for reversed Z

            mCommandList->SetGraphicsRoot32BitConstants(RootParameter_Shadow::ShadowMatrix_Index, 1, &matrixIndex, 0);

            CullObjectsForShadow(light, cascadeIndex);
            Render_Objects(mCommandList, RootParameter_Shadow::ObjectCBV, mVisibleItems);
        }
    }

    fr.StateTracker.Transition(mCommandList.Get(), lr.CsmShadowArray.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // =======================================================
    // C. Spot
    D3D12_VIEWPORT spotViewport = LightComponent::Get_ShadowMapViewport(Light_Type::Spot);
    D3D12_RECT spotScissor = LightComponent::Get_ShadowMapScissorRect(Light_Type::Spot);

    mCommandList->RSSetViewports(1, &spotViewport);
    mCommandList->RSSetScissorRects(1, &spotScissor);

    fr.StateTracker.Transition(mCommandList.Get(), lr.SpotShadowArray.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

    for (UINT i = 0; i < lr.mFrameShadowCastingSpot.size(); ++i)
    {
        LightComponent* light = lr.mFrameShadowCastingSpot[i];
        UINT matrixIndex = lr.mLightShadowIndexMap[light];

        auto dsv = mDsvManager->GetCpuHandle(lr.SpotShadow_DSVs[i]);
        mCommandList->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
        mCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr); // Clear to far plane 0.0f for reversed Z

        mCommandList->SetGraphicsRoot32BitConstants(RootParameter_Shadow::ShadowMatrix_Index, 1, &matrixIndex, 0);

        CullObjectsForShadow(light, 0);
        Render_Objects(mCommandList, RootParameter_Shadow::ObjectCBV, mVisibleItems);
    }

    fr.StateTracker.Transition(mCommandList.Get(), lr.SpotShadowArray.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void DX12_Renderer::CompositePass(std::shared_ptr<CameraComponent> render_camera)
{
    FrameResource& fr = mFrameResources[mFrameIndex];
    LightResource& lr = fr.light_resource;

    ID3D12RootSignature* rs = RootSignatureFactory::Get(RootSignature_Type::PostFX);
    mCommandList->SetGraphicsRootSignature(rs);

    PSO_Manager::Instance().BindShader(mCommandList, "Composite", ShaderVariant::Default);

    //============================================

    PrepareGBuffer_SRV();

    fr.StateTracker.Transition(mCommandList.Get(), fr.Merge_RenderTargets[fr.Merge_Target_Index].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);

    fr.StateTracker.Transition(mCommandList.Get(), lr.ClusterLightMetaBuffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    fr.StateTracker.Transition(mCommandList.Get(), lr.ClusterLightIndicesBuffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    auto rtv = mRtvManager->GetCpuHandle(fr.MergeRtvSlot_IDs[fr.Merge_Target_Index]);
    mCommandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    
    Bind_SceneCBV(Shader_Type::Graphics, RootParameter_PostFX::SceneCBV);

    render_camera->Graphics_Bind(mCommandList, RootParameter_PostFX::CameraCBV);

    auto ClusterSrv = mResource_Heap_Manager->GetGpuHandle(lr.ClusterBuffer_SRV_Index);
    mCommandList->SetGraphicsRootDescriptorTable(RootParameter_PostFX::ClusterAreaSRV, ClusterSrv);

    auto LightSrv = mResource_Heap_Manager->GetGpuHandle(lr.LightBuffer_SRV_Index);
    mCommandList->SetGraphicsRootDescriptorTable(RootParameter_PostFX::LightBufferSRV, LightSrv);

    auto LightMetaSrv = mResource_Heap_Manager->GetGpuHandle(lr.ClusterLightMetaBuffer_SRV_Index);
    mCommandList->SetGraphicsRootDescriptorTable(RootParameter_PostFX::ClusterLightMetaSRV, LightMetaSrv);

    auto LightIndicesSrv = mResource_Heap_Manager->GetGpuHandle(lr.ClusterLightIndicesBuffer_SRV_Index);
    mCommandList->SetGraphicsRootDescriptorTable(RootParameter_PostFX::ClusterLightIndicesSRV, LightIndicesSrv);


    if (fr.GBufferSrvSlot_IDs.size() >= (UINT)GBufferType::Count)
    {
        auto gbufferSrv = mResource_Heap_Manager->GetGpuHandle(fr.GBufferSrvSlot_IDs[0]); 
        mCommandList->SetGraphicsRootDescriptorTable(RootParameter_PostFX::GBufferTable, gbufferSrv);
    }

    if (fr.DepthBufferSrvSlot_ID != UINT_MAX)
    {
        auto depthSrv = mResource_Heap_Manager->GetGpuHandle(fr.DepthBufferSrvSlot_ID);
        mCommandList->SetGraphicsRootDescriptorTable(RootParameter_PostFX::DepthTexture, depthSrv);
    }

    {
        auto ShadowMatrixSrv = mResource_Heap_Manager->GetGpuHandle(lr.ShadowMatrixBuffer_SRV_Index);
        mCommandList->SetGraphicsRootDescriptorTable(RootParameter_PostFX::ShadowMatrix_SRV, ShadowMatrixSrv);

        auto csmSrv = mResource_Heap_Manager->GetGpuHandle(lr.CsmShadowArray_SRV);
        mCommandList->SetGraphicsRootDescriptorTable(RootParameter_PostFX::ShadowMapCSMTable, csmSrv);

        auto spotSrv = mResource_Heap_Manager->GetGpuHandle(lr.SpotShadowArray_SRV);
        mCommandList->SetGraphicsRootDescriptorTable(RootParameter_PostFX::ShadowMapSpotTable, spotSrv);

        auto pointSrv = mResource_Heap_Manager->GetGpuHandle(lr.PointShadowCubeArray_SRV);
        mCommandList->SetGraphicsRootDescriptorTable(RootParameter_PostFX::ShadowMapPointTable, pointSrv);
    }

    auto src = mResource_Heap_Manager->GetGpuHandle(fr.MergeSrvSlot_IDs[fr.Merge_Base_Index]);
    mCommandList->SetGraphicsRootDescriptorTable(RootParameter_PostFX::MergeTexture, src);

    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCommandList->DrawInstanced(6, 1, 0, 0);

    std::swap(fr.Merge_Base_Index, fr.Merge_Target_Index);

    fr.StateTracker.Transition(mCommandList.Get(), fr.light_resource.ClusterBuffer.Get(), D3D12_RESOURCE_STATE_COMMON);
    fr.StateTracker.Transition(mCommandList.Get(), fr.light_resource.ClusterLightMetaBuffer.Get(), D3D12_RESOURCE_STATE_COMMON);
    fr.StateTracker.Transition(mCommandList.Get(), fr.light_resource.ClusterLightIndicesBuffer.Get(), D3D12_RESOURCE_STATE_COMMON);
}

void DX12_Renderer::PostProcessPass(std::shared_ptr<CameraComponent> render_camera)
{
    FrameResource& fr = mFrameResources[mFrameIndex];

    ID3D12RootSignature* rs = RootSignatureFactory::Get(RootSignature_Type::PostFX);
    mCommandList->SetGraphicsRootSignature(rs);

    PSO_Manager::Instance().BindShader(mCommandList, "PostProcess", ShaderVariant::Default);

    //============================================

    fr.StateTracker.Transition(mCommandList.Get(), fr.Merge_RenderTargets[fr.Merge_Base_Index].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    fr.StateTracker.Transition(mCommandList.Get(), fr.Merge_RenderTargets[fr.Merge_Target_Index].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto rtv = mRtvManager->GetCpuHandle(fr.MergeRtvSlot_IDs[fr.Merge_Target_Index]);
    mCommandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    Bind_SceneCBV(Shader_Type::Graphics, RootParameter_PostFX::SceneCBV);

    render_camera->Graphics_Bind(mCommandList, RootParameter_Default::CameraCBV);


    if (fr.GBufferSrvSlot_IDs.size() >= (UINT)GBufferType::Count)
    {
        auto gbufferSrv = mResource_Heap_Manager->GetGpuHandle(fr.GBufferSrvSlot_IDs[0]);
        mCommandList->SetGraphicsRootDescriptorTable(RootParameter_PostFX::GBufferTable, gbufferSrv);
    }

    if (fr.DepthBufferSrvSlot_ID != UINT_MAX)
    {
        auto depthSrv = mResource_Heap_Manager->GetGpuHandle(fr.DepthBufferSrvSlot_ID);
        mCommandList->SetGraphicsRootDescriptorTable(RootParameter_PostFX::DepthTexture, depthSrv);
    }

    auto src = mResource_Heap_Manager->GetGpuHandle(fr.MergeSrvSlot_IDs[fr.Merge_Base_Index]);
    mCommandList->SetGraphicsRootDescriptorTable(RootParameter_PostFX::MergeTexture, src);

    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCommandList->DrawInstanced(6, 1, 0, 0);

    std::swap(fr.Merge_Base_Index, fr.Merge_Target_Index);
}


void DX12_Renderer::Blit_BackBufferPass()
{
    FrameResource& fr = mFrameResources[mFrameIndex];

    ID3D12RootSignature* rs = RootSignatureFactory::Get(RootSignature_Type::PostFX);
    mCommandList->SetGraphicsRootSignature(rs);
    PSO_Manager::Instance().BindShader(mCommandList, "Blit", ShaderVariant::Default);

    //============================================

    fr.StateTracker.Transition(mCommandList.Get(), fr.RenderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);

    fr.StateTracker.Transition(mCommandList.Get(), fr.Merge_RenderTargets[fr.Merge_Base_Index].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    auto rtv = mRtvManager->GetCpuHandle(fr.BackBufferRtvSlot_ID);
    mCommandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    Bind_SceneCBV(Shader_Type::Graphics, RootParameter_PostFX::SceneCBV);

    if (fr.GBufferSrvSlot_IDs.size() >= (UINT)GBufferType::Count)
    {
        auto gbufferSrv = mResource_Heap_Manager->GetGpuHandle(fr.GBufferSrvSlot_IDs[0]);
        mCommandList->SetGraphicsRootDescriptorTable(RootParameter_PostFX::GBufferTable, gbufferSrv);
    }

    if (fr.DepthBufferSrvSlot_ID != UINT_MAX)
    {
        auto depthSrv = mResource_Heap_Manager->GetGpuHandle(fr.DepthBufferSrvSlot_ID);
        mCommandList->SetGraphicsRootDescriptorTable(RootParameter_PostFX::DepthTexture, depthSrv);
    }

    auto src = mResource_Heap_Manager->GetGpuHandle(fr.MergeSrvSlot_IDs[fr.Merge_Base_Index]);
    mCommandList->SetGraphicsRootDescriptorTable(RootParameter_PostFX::MergeTexture, src);

    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCommandList->DrawInstanced(6, 1, 0, 0);
}

void DX12_Renderer::ImguiPass()
{
    GameTimer* gt = GameEngine::Get().GetTimer();

    unsigned long fps = gt->GetFrameRate();

    bool use_imgui = true;
    if (use_imgui)
    {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Performance");
        ImGui::Text("FPS: %lu", fps);
        ImGui::End();

        // ------------------ Scene Hierarchy ------------------
        auto scene = GameEngine::Get().GetActiveScene();
        if (scene)
        {
            ImGui::Begin("Scene Hierarchy");
            std::vector<Object*> root_obj_list = scene->GetRootObjectList();

            for (auto& root : root_obj_list)
                DrawObjectNode(root);
            ImGui::End();
        }

        // ------------------ Inspector ------------------
        ImGui::Begin("Inspector");
        auto selected = GameEngine::Get().GetSelectedObject();
        DrawInspector(selected);
        ImGui::End();


        ImGui::Render();



        // ---- ImGui 드로우 ----
        ID3D12DescriptorHeap* imgui_heaps[] = { mImguiSrvHeap.Get() };
        mCommandList->SetDescriptorHeaps(1, imgui_heaps);

        FrameResource& fr = mFrameResources[mFrameIndex];
        auto rtv = mRtvManager->GetCpuHandle(fr.BackBufferRtvSlot_ID);
        auto dsv = mDsvManager->GetCpuHandle(fr.DsvSlot_ID);

        mCommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());

        // Viewport 지원
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
}


// ------------------- Utility -------------------
void DX12_Renderer::WaitForFrame(UINT64 fenceValue)
{
    if (fenceValue == 0) return;
    if (mFence->GetCompletedValue() < fenceValue)
    {
        mFence->SetEventOnCompletion(fenceValue, mFenceEvent);
        WaitForSingleObject(mFenceEvent, INFINITE);
    }
}

bool DX12_Renderer::OnResize(UINT newWidth, UINT newHeight)
{
    if (!is_initialized) return false;
    if (!mDevice || !mSwapChain) return false;

    for (auto& fr : mFrameResources)
        if (fr.FenceValue > 0)
            WaitForFrame(fr.FenceValue);

    mWidth = newWidth;
    mHeight = newHeight;

    for (UINT i = 0; i < FrameCount; ++i)
    {
        mFrameResources[i].RenderTarget.Reset();
        DestroySingleFrameResource(mFrameResources[i]);
    }

    mRtvManager->Update();
    mDsvManager->Update();
    mResource_Heap_Manager->Update();

    DXGI_SWAP_CHAIN_DESC desc;
    mSwapChain->GetDesc(&desc);
    HRESULT hr = mSwapChain->ResizeBuffers(FrameCount, newWidth, newHeight, desc.BufferDesc.Format, desc.Flags);
    if (FAILED(hr))
    {
        return false;
    }
    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    return CreateFrameResources();
}


void DX12_Renderer::Cleanup()
{
    for (UINT i = 0; i < FrameCount; i++)
        WaitForFrame(mFrameResources[i].FenceValue);

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CloseHandle(mFenceEvent);
}

RendererContext DX12_Renderer::Get_RenderContext() const
{
    RendererContext ctx = {};
    ctx.device = mDevice.Get();
    ctx.cmdList = mCommandList.Get();
    ctx.resourceHeap = mResource_Heap_Manager.get();
    return ctx;
}

RendererContext DX12_Renderer::Get_UploadContext() const
{
    RendererContext ctx = {};
    ctx.device = mDevice.Get();
    ctx.cmdList = mUploadCommandList.Get();
    ctx.resourceHeap = mResource_Heap_Manager.get();
    return ctx;
}

ImGui_ImplDX12_InitInfo DX12_Renderer::GetImGuiInitInfo() const
{
    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = mDevice.Get();
    init_info.CommandQueue = mCommandQueue.Get();
    init_info.NumFramesInFlight = FrameCount;
    init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    init_info.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    init_info.SrvDescriptorHeap = mImguiSrvHeap.Get();

    init_info.LegacySingleSrvCpuDescriptor = mImguiSrvHeap->GetCPUDescriptorHandleForHeapStart();
    init_info.LegacySingleSrvGpuDescriptor = mImguiSrvHeap->GetGPUDescriptorHandleForHeapStart();

    return init_info;
}


void DrawObjectNode(Object* obj)
{
    if (!obj) return;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (obj->GetChildren().empty())
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    std::string label = obj->GetName();
    bool opened = ImGui::TreeNodeEx((void*)(intptr_t)obj->GetId(), flags, "%s", label.c_str());

    if (ImGui::IsItemClicked())
        GameEngine::Get().SelectObject(obj);

    if (opened && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
    {
        for (auto& child : obj->GetChildren())
            DrawObjectNode(child);
        ImGui::TreePop();
    }
}

void DrawInspector(Object* obj)
{
    if (!obj)
    {
        ImGui::Text("No Object Selected");
        return;
    }

    ImGui::Text("Name: %s", obj->GetName().c_str());
    ImGui::Text("ID: %u", obj->GetId());

    if (auto tf = obj->GetTransform())
    {
        ImGui::Text("\n");
        ImGui::Text("Transform:");
        XMFLOAT3 pos = tf->GetPosition();
        XMFLOAT3 rot = tf->GetRotationEuler();
        XMFLOAT3 scale = tf->GetScale();

        if (ImGui::DragFloat3("Position", reinterpret_cast<float*>(&pos), 0.01f))
            tf->SetPosition(pos);
        if (ImGui::DragFloat3("Rotation", reinterpret_cast<float*>(&rot), 0.1f))
            tf->SetRotationEuler(rot);
        if (ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&scale), 0.01f))
            tf->SetScale(scale);
    }

    ImGui::Separator();
    ImGui::Text("Components:");

    std::unordered_map<Component_Type, std::vector<std::shared_ptr<Component>>> component_list = obj->GetAllComponents();

    for (auto& [type, comps] : component_list)
    {
        if (comps.empty()) continue;

        std::string component_name = "";
        switch (type)
        {
        case    Mesh_Renderer: component_name = "Mesh_Renderer"; break;
        case    Camera: component_name = "Camera"; break;
        case    Collider:   component_name = "Collider"; break;
        case    Rigidbody:  component_name = "Rigidbody"; break;
        case    Light:  component_name = "Light"; break;

        case    Transform:
        case    etc:
        default:
            continue;
        }

        if (ImGui::CollapsingHeader(component_name.data(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (auto& comp : comps)
            {
                DrawComponentInspector(comp);
            }
        }
    }
}

void DrawComponentInspector(const std::shared_ptr<Component>& comp)
{
    Component_Type component_type = comp->GetType();

    switch (component_type)
    {

    case Mesh_Renderer:
    {
        std::shared_ptr<MeshRendererComponent> mr = std::dynamic_pointer_cast<MeshRendererComponent>(comp);
        if (mr)
        {
            ResourceSystem* rsm = GameEngine::Get().GetResourceSystem();

            ImGui::Text("MeshRenderer");
            ImGui::Separator();
            ImGui::Indent();

            // Get mesh info
            auto meshPtr = mr->GetMesh();
            if (meshPtr)
            {
                ImGui::Text("Mesh Info");
                ImGui::Separator();

                if (ImGui::BeginTable("MeshTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                {
                    ImGui::TableSetupColumn("Property");
                    ImGui::TableSetupColumn("Value");
                    ImGui::TableHeadersRow();

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("Mesh Alias");
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%s", meshPtr->GetAlias().c_str());
                    
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("Mesh ID");
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%u", mr->GetMesh()->GetId());


                    ImGui::EndTable();
                }

                // Submesh material list
                ImGui::Spacing();
                ImGui::Text("Submesh Materials");
                ImGui::Separator();

                if (ImGui::BeginTable("MaterialTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                {
                    ImGui::TableSetupColumn("Submesh Index");
                    ImGui::TableSetupColumn("Material Alias");
                    ImGui::TableSetupColumn("Material ID");
                    ImGui::TableHeadersRow();

                    const auto& submeshes = meshPtr->submeshes;
                    for (size_t i = 0; i < submeshes.size(); ++i)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%zu", i);

                        UINT matId = mr->GetMaterial(i);
                        std::string matAlias = "<null>";

                        if (matId != Engine::INVALID_ID)
                        {
                            auto matPtr = rsm->GetById<Material>(matId);
                            if (matPtr)
                                matAlias = matPtr->GetAlias();
                        }

                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%u", matId);

                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%s", matAlias.c_str());
                    }

                    ImGui::EndTable();
                }
            }
            else
            {
                ImGui::Text("Mesh: <invalid>");
            }

            ImGui::Unindent();
        }
    }
    break;


    case Camera:
    {
        std::shared_ptr<CameraComponent> cam = dynamic_pointer_cast<CameraComponent>(comp);
        if (cam)
        {
            bool useTarget = cam->GetTargetUse();
            XMFLOAT3 target_pos = cam->GetTargetPosition();

            float FovY = cam->GetFovY();
            float mNearZ = cam->GetNearZ();
            float mFarZ = cam->GetFarZ();


            if (ImGui::Checkbox("Use Target Focus", &useTarget))
                cam->SetTargetUse(useTarget);

            if (ImGui::DragFloat3("Focus Target", reinterpret_cast<float*>(&target_pos), 0.01f))
                cam->SetTargetPosition(target_pos);

            if (ImGui::DragFloat("FovY", &FovY, 0.1f, 0.1f, 100.0f))
                cam->SetFovY(FovY);

            if (ImGui::DragFloat("NearZ", &mNearZ, 0.1f, 0.1f, 100.0f))
                cam->SetNearZ(mNearZ);

            if (ImGui::DragFloat("FarZ", &mFarZ, 0.1f, 0.1f, 100.0f))
                cam->SetFarZ(mFarZ);
        }
    }
    break;

    case Light:
    {
        std::shared_ptr<LightComponent> light = std::dynamic_pointer_cast<LightComponent>(comp);
        if (light)
        {
            ImGui::Text("Light Component");
            ImGui::Separator();
            ImGui::Indent();

            Light_Type type = light->GetLightType();
            const char* typeNames[] = { "Directional", "Point", "Spot" };
            int currentType = static_cast<int>(type);

            // ==========================================================
            // 1. General Light Settings
            // ==========================================================
            ImGui::Text("Light Settings");
            ImGui::Separator();

            if (ImGui::Combo("Light Type", &currentType, typeNames, IM_ARRAYSIZE(typeNames)))
                light->SetLightType(static_cast<Light_Type>(currentType));

            XMFLOAT3 color = light->GetColor();
            if (ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&color)))
                light->SetColor(color);

            float intensity = light->GetIntensity();
            if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 1000.0f))
                light->SetIntensity(intensity);

            if (type == Light_Type::Directional || type == Light_Type::Spot)
            {
                XMFLOAT3 direction = light->GetDirection();
                if (ImGui::DragFloat3("Direction", reinterpret_cast<float*>(&direction), 0.01f, -1.0f, 1.0f))
                {
                    if (direction.x == 0.0f && direction.y == 0.0f && direction.z == 0.0f)
                        direction.y = -1.0f;
                    light->SetDirection(direction);
                }
            }

            if (type == Light_Type::Point || type == Light_Type::Spot)
            {
                float range = light->GetRange();
                if (ImGui::DragFloat("Range", &range, 0.1f, 0.1f, 10000.0f))
                    light->SetRange(range);
            }

            if (type == Light_Type::Spot)
            {
                float inner = XMConvertToDegrees(light->GetInnerAngle());
                if (ImGui::DragFloat("Inner Angle", &inner, 0.5f, 0.0f, 90.0f))
                    light->SetInnerAngle(XMConvertToRadians(inner));

                float outer = XMConvertToDegrees(light->GetOuterAngle());
                if (ImGui::DragFloat("Outer Angle", &outer, 0.5f, 0.0f, 180.0f))
                    light->SetOuterAngle(XMConvertToRadians(outer));
            }

            ImGui::Spacing();

            // ==========================================================
            // 2. Shadow Settings
            // ==========================================================
            ImGui::Text("Shadow Settings");
            ImGui::Separator();

            bool castShadow = light->CastsShadow();
            ImGui::Checkbox("Cast Shadow", &castShadow);
            light->SetCastShadow(castShadow);

            if (castShadow)
            {
                ShadowMode shadowMode = light->GetShadowMode();
                const char* modeNames[] = { "Dynamic", "Static" };
                int currentModeInt = static_cast<int>(shadowMode);

                if (ImGui::Combo("Shadow Mode", &currentModeInt, modeNames, IM_ARRAYSIZE(modeNames)))
                {
                    light->SetShadowMode(static_cast<ShadowMode>(currentModeInt));
                }

                if (static_cast<ShadowMode>(currentModeInt) == ShadowMode::Static)
                {
                    if (ImGui::Button("Bake New ShadowMap"))
                    {
                        light->ForceShadowMapUpdate();
                    }
                    ImGui::SameLine();
                    ImGui::TextDisabled("(One-time manual bake)");
                }

                ImGui::Separator();

                if (type == Light_Type::Directional)
                {
                    DirectionalShadowMode dirMode = light->GetDirectionalShadowMode();
                    const char* dirModeNames[] = { "Default (StaticGlobal)", "CSM (Camera Dependent)" };
                    int currentDirModeInt = static_cast<int>(dirMode);

                    if (ImGui::Combo("Directional Mode", &currentDirModeInt, dirModeNames, IM_ARRAYSIZE(dirModeNames)))
                    {
                        light->SetDirectionalShadowMode(static_cast<DirectionalShadowMode>(currentDirModeInt));
                    }

                    if (static_cast<DirectionalShadowMode>(currentDirModeInt) == DirectionalShadowMode::Default)
                    {
                        float orthoSize = light->GetStaticOrthoSize();
                        if (ImGui::DragFloat("Static Ortho Size", &orthoSize, 1.0f, 1.0f, 10000.0f))
                        {
                            light->SetStaticOrthoSize(orthoSize);
                        }
                    }

                    ImGui::Separator();

                    float lambda = light->GetCascadeLambda();
                    if (ImGui::SliderFloat("Cascade Lambda", &lambda, 0.0f, 1.0f, "%.2f"))
                        light->SetCascadeLambda(lambda);

                    ImGui::TextDisabled("0.0 = Uniform, 1.0 = Logarithmic");

                    ImGui::Separator();
                }

                float shadowNear = light->GetShadowMapNear();
                if (ImGui::DragFloat("Shadow Near", &shadowNear, 0.1f, 0.01f, light->GetShadowMapFar() - 1.0f))
                    light->SetShadowMapNear(shadowNear);

                float shadowFar = light->GetShadowMapFar();
                if (ImGui::DragFloat("Shadow Far", &shadowFar, 1.0f, light->GetShadowMapNear() + 1.0f, 20000.0f))
                    light->SetShadowMapFar(shadowFar);
            }

            ImGui::Unindent();
        }
    }
    break;




    case Collider:
        break;
    case Rigidbody:
    {
        std::shared_ptr<RigidbodyComponent> rb = dynamic_pointer_cast<RigidbodyComponent>(comp);
        if (rb)
        {
            ImGui::Text("Rigidbody");
            ImGui::Indent();
            float mass = rb->GetMass();
            bool useGravity = rb->GetUseGravity();
            XMFLOAT3 GravityValue = rb->GetGravity();

            if (ImGui::DragFloat("Mass", &mass, 0.1f, 0.1f, 100.0f))
                rb->SetMass(mass);
            if (ImGui::Checkbox("Use Gravity", &useGravity))
                rb->SetUseGravity(useGravity);
            if (ImGui::DragFloat3("Gravity Value", reinterpret_cast<float*>(&GravityValue), 0.01f))
                rb->SetGravity(GravityValue);

            ImGui::Unindent();
        } 
    }
    break;

    case Transform:
    case etc:
    default:
        ImGui::Text("Unknown Component");
        break;
    }
}
