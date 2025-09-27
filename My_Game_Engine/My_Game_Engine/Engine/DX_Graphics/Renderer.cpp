#include "pch.h"
#include "Renderer.h"
#include "GameEngine.h"
#include "../Resource/Mesh.h"
#include "../Resource/Material.h"
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

//=======================================================================

float DX12_Renderer::clear_color[4] = { 0.95f, 0.55f, 0.60f, 1.00f };

bool DX12_Renderer::Initialize(HWND hWnd, UINT width, UINT height)
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

    if (!CreateDeviceAndFactory()) return false;
    if (!CheckMsaaSupport()) return false;
    if (!CreateCommandQueue()) return false;
    if (!CreateSwapChain(hWnd, width, height)) return false;
    if (!CreateDescriptorHeaps()) return false;
    if (!CreateFrameResources()) return false;
    if (!CreateCommandList()) return false;
    if (!CreateFenceObjects()) return false;
    if (!CreateCommandList_Upload()) return false;
    
    if (!Create_Shader()) return false;
    if (!Create_SceneCBV()) return false;


    //---------------------------------------------------------------------
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1; 
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;

    if (FAILED(mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mImguiSrvHeap))))
        return false;

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

bool DX12_Renderer::CreateSwapChain(HWND hWnd, UINT width, UINT height)
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
        mCommandQueue.Get(), hWnd, &desc, nullptr, nullptr, &swapChain)))
        return false;

    mFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);

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
    mDsvManager = std::make_unique<DescriptorManager>(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, FrameCount);
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
    for (UINT i = 0; i < FrameCount; i++)
    {
        FrameResource& fr = mFrameResources[i];

        if (!CreateCommandAllocator(fr)) return false;
        if (!CreateBackBufferRTV(i, fr)) return false;
        if (!CreateDSV(i, fr)) return false;
        if (!CreateGBuffer(i, fr)) return false;
        if (!Create_Merge_RenderTargets(i, fr)) return false;
        if (!CreateObjectCB(fr, 1000)) return false;
    }
    return true;
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


bool DX12_Renderer::CreateGBuffer(UINT frameIndex, FrameResource& frame)
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

    if (!CreateGBufferRTVs(frameIndex, frame)) return false;
    if (!CreateGBufferSRVs(frameIndex, frame)) return false;


    return true;
}

bool DX12_Renderer::CreateGBufferRTVs(UINT frameIndex, FrameResource& fr)
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

bool DX12_Renderer::CreateGBufferSRVs(UINT frameIndex, FrameResource& fr)
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
        dSrv.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

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

    auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R24G8_TYPELESS, width, height, 1, 1);
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // Clear value for depth/stencil
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    clearValue.DepthStencil.Depth = 1.0f;
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

bool DX12_Renderer::CreateDSV(UINT frameIndex, FrameResource& fr)
{
    if (!CreateDepthStencil(fr, mWidth, mHeight)) return false;

    UINT slot = mDsvManager->Allocate(HeapRegion::DSV);
    fr.DsvSlot_ID = slot;

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; 
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    mDevice->CreateDepthStencilView(fr.DepthStencilBuffer.Get(), &dsvDesc, mDsvManager->GetCpuHandle(slot));

    fr.StateTracker.Register(fr.DepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

    return true;
}


bool DX12_Renderer::Create_Merge_RenderTargets(UINT frameIndex, FrameResource& fr)
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
    geometry_ss.vs.file = L"shaders/Geometry_Shader.hlsl";
    geometry_ss.vs.entry = "Default_VS";
    geometry_ss.vs.target = "vs_5_1";

    geometry_ss.ps.file = L"shaders/Geometry_Shader.hlsl";
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

    ShaderSetting composite_ss;
    composite_ss.vs.file = L"shaders/Composite_Shader.hlsl";
    composite_ss.vs.entry = "Default_VS";
    composite_ss.vs.target = "vs_5_1";

    composite_ss.ps.file = L"shaders/Composite_Shader.hlsl";
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
    postprocess_ss.vs.file = L"shaders/PostProcess_Shader.hlsl";
    postprocess_ss.vs.entry = "Default_VS";
    postprocess_ss.vs.target = "vs_5_1";

    postprocess_ss.ps.file = L"shaders/PostProcess_Shader.hlsl";
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
    blit_ss.vs.file = L"shaders/Blit_Shader.hlsl";
    blit_ss.vs.entry = "Default_VS";
    blit_ss.vs.target = "vs_5_1";

    blit_ss.ps.file = L"shaders/Blit_Shader.hlsl";
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

void DX12_Renderer::Update_SceneCBV()
{
    SceneData cb{};

    memcpy(mappedSceneDataCB, &cb, sizeof(SceneData));
}

void DX12_Renderer::UpdateObjectCBs(const std::vector<RenderData>& renderables)
{
    FrameResource& fr = mFrameResources[mFrameIndex];
    fr.ObjectCB.HeadOffset = 0;

    int i = 0;
    for (auto& rd : renderables)
    {
        auto transform = rd.transform.lock();
        auto renderer = rd.meshRenderer.lock();

        if (!transform || !renderer)
            continue;

        if (fr.ObjectCB.HeadOffset >= fr.ObjectCB.MaxObjects)
            break;

        auto mesh = renderer->GetMesh();
        if (!mesh)
            continue;

        UINT material_id = mesh->GetMaterialID();
        ResourceManager* rm = GameEngine::Get().GetResourceManager();
        auto material = rm->GetById<Material>(material_id);

        if (!material)
            continue;

        ObjectCBData cb{};
        cb.World = transform->GetWorldMatrix();

        cb.Albedo = XMFLOAT4(material->albedoColor.x, material->albedoColor.y, material->albedoColor.z, 1.0f);
        cb.Roughness = material->roughness;
        cb.Metallic = material->metallic;
        cb.Emissive = 0.0f; 

        auto toIdx = [](UINT slot) -> int {
            return (slot == UINT_MAX) ? -1 : static_cast<int>(slot); 
            };

       

        cb.DiffuseTexIdx = toIdx(material->diffuseTexSlot);
        cb.NormalTexIdx = toIdx(material->normalTexSlot);
        cb.RoughnessTexIdx = toIdx(material->roughnessTexSlot);
        cb.MetallicTexIdx = toIdx(material->metallicTexSlot);

        // Test 0
        //if (i == 8)
        //{
        //    OutputDebugStringA(mesh->GetAlias().data());
        //    OutputDebugStringA("\n");

        //    cb.DiffuseTexIdx = toIdx(2);
        //}


        // Test 1
        //if (i  == 6)
        //{
        //    OutputDebugStringA(mesh->GetAlias().data());
        //    OutputDebugStringA("\n");

        //    cb.DiffuseTexIdx = toIdx(1);
        //}

        // Test 2
        //if (i == 7)
        //{
        //    OutputDebugStringA(mesh->GetAlias().data());
        //    OutputDebugStringA("\n");

        //    cb.DiffuseTexIdx = toIdx(1);
        //}


        fr.ObjectCB.MappedObjectCB[fr.ObjectCB.HeadOffset] = cb;
        transform->SetCbOffset(mFrameIndex, fr.ObjectCB.HeadOffset);
        i++;
        fr.ObjectCB.HeadOffset++;
    }
}

void DX12_Renderer::SortByRenderType(std::vector<RenderData> renderData_list)
{

}

void DX12_Renderer::Render_Objects(ComPtr<ID3D12GraphicsCommandList> cmdList, const std::vector<RenderData>& renderData_list)
{
    FrameResource& fr = mFrameResources[mFrameIndex];
    for (auto& rd : renderData_list)
    {
        auto renderer = rd.meshRenderer.lock();
        auto transform = rd.transform.lock();

        if (!renderer || !transform)
            continue;

        auto mesh = renderer->GetMesh();
        if (!mesh)
            continue;

        UINT offset = transform->GetCbOffset(mFrameIndex);
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = fr.ObjectCB.Buffer->GetGPUVirtualAddress() + offset * sizeof(ObjectCBData);

        cmdList->SetGraphicsRootConstantBufferView(RootParameter_Default::ObjectCBV, gpuAddr);


        mesh->Bind(cmdList);
        cmdList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
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

    auto dsv = mDsvManager->GetCpuHandle(fr.DsvSlot_ID);

    for (auto& target : fr.gbuffer.targets)
        fr.StateTracker.Transition(mCommandList.Get(), target.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);

    mCommandList->OMSetRenderTargets((UINT)rtvs.size(), rtvs.data(), FALSE, &dsv);

    //===================

    for (UINT g = 0; g < (UINT)GBufferType::Count; g++)
    {
        const MRTTargetDesc& cfg = GBUFFER_CONFIG[g];
        mCommandList->ClearRenderTargetView(rtvs[g], cfg.clearColor, 0, nullptr);
    }

    mCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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

    mSwapChain->Present(1, 0);

    const UINT64 currentFence = ++mFenceValue;
    mCommandQueue->Signal(mFence.Get(), currentFence);

    mFrameFenceValues[mFrameIndex] = currentFence;
}

void DX12_Renderer::Render(std::vector<RenderData> renderData_list, std::shared_ptr<CameraComponent> render_camera)
{
    PrepareCommandList();
    
    ID3D12DescriptorHeap* heaps[] = { mResource_Heap_Manager->GetHeap() };
    mCommandList->SetDescriptorHeaps(1, heaps);

    render_camera->Update();
    render_camera->SetViewportsAndScissorRects(mCommandList);

    UpdateObjectCBs(renderData_list);

    GeometryPass(renderData_list, render_camera);

    CompositePass();

    PostProcessPass();

    Blit_BackBufferPass();

//    ImguiPass();
    TransitionBackBufferToPresent();
    mCommandList->Close();
    PresentFrame();

}

void DX12_Renderer::GeometryPass(std::vector<RenderData> renderData_list, std::shared_ptr<CameraComponent> render_camera)
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
    mCommandList->SetGraphicsRootConstantBufferView(RootParameter_Default::SceneCBV, mSceneData_CB->GetGPUVirtualAddress());


    //mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //mCommandList->DrawInstanced(6, 1, 0, 0);

    render_camera->UpdateCBV();
    render_camera->Bind(mCommandList, RootParameter_Default::CameraCBV);

    Render_Objects(mCommandList, renderData_list);
}

void DX12_Renderer::CompositePass()
{
    FrameResource& fr = mFrameResources[mFrameIndex];

    ID3D12RootSignature* rs = RootSignatureFactory::Get(RootSignature_Type::PostFX);
    mCommandList->SetGraphicsRootSignature(rs);

    PSO_Manager::Instance().BindShader(mCommandList, "Composite", ShaderVariant::Default);

    //============================================



    PrepareGBuffer_SRV();

    fr.StateTracker.Transition(mCommandList.Get(), fr.Merge_RenderTargets[fr.Merge_Target_Index].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto rtv = mRtvManager->GetCpuHandle(fr.MergeRtvSlot_IDs[fr.Merge_Target_Index]);
    mCommandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    

    mCommandList->SetGraphicsRootConstantBufferView(RootParameter_Default::SceneCBV, mSceneData_CB->GetGPUVirtualAddress());

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

    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCommandList->DrawInstanced(6, 1, 0, 0);

    std::swap(fr.Merge_Base_Index, fr.Merge_Target_Index);
}

void DX12_Renderer::PostProcessPass()
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

    mCommandList->SetGraphicsRootConstantBufferView(RootParameter_Default::SceneCBV, mSceneData_CB->GetGPUVirtualAddress());

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


    mCommandList->SetGraphicsRootConstantBufferView(RootParameter_Default::SceneCBV, mSceneData_CB->GetGPUVirtualAddress());

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
    bool use_imgui = true;
    if (use_imgui)
    {
        // ImGui 시작
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();


        ImGui::Begin("Color Controller");
        ImGui::ColorEdit4("MyColor RGBA", clear_color);
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
    if (!mDevice || !mSwapChain) return false;

    for (auto& fr : mFrameResources)
        WaitForFrame(fr.FenceValue);

    mWidth = newWidth;
    mHeight = newHeight;

    for (auto& fr : mFrameResources) {
        fr.RenderTarget.Reset();
        fr.DepthStencilBuffer.Reset();
        for (auto& target : fr.gbuffer.targets)
            target.Reset();
        fr.gbuffer.Depth.Reset();
    }

    DXGI_SWAP_CHAIN_DESC desc;
    mSwapChain->GetDesc(&desc);
    mSwapChain->ResizeBuffers(FrameCount, newWidth, newHeight, desc.BufferDesc.Format, desc.Flags);
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
