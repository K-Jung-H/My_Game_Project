#include "pch.h"
#include "Renderer.h"
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
    UINT rtvCount = FrameCount + FrameCount * (UINT)GBufferType::Count;
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
        if (!CreateGBufferRTVs(i, fr)) return false;
        if (!CreateDSV(i, fr)) return false;
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

    UINT slot = mRtvManager->Allocate();
    D3D12_CPU_DESCRIPTOR_HANDLE handle = mRtvManager->GetCpuHandle(slot);

    mDevice->CreateRenderTargetView(fr.RenderTarget.Get(), nullptr, handle);
    mBackBufferRtvHandles[frameIndex] = handle;


    fr.StateTracker.Register(fr.RenderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT);

    return true;
}

bool DX12_Renderer::CreateGBuffer(FrameResource& frame, UINT width, UINT height)
{
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    for (UINT i = 0; i < (UINT)GBufferType::Count; i++) {
        const MRTTargetDesc& desc = GBUFFER_CONFIG[i];

        // Create Tex2D resource description
        auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(desc.format, width, height, 1, 1);
        texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        // Clear value
        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = desc.format;
        memcpy(clearValue.Color, desc.clearColor, sizeof(FLOAT) * 4);

        if (FAILED(mDevice->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, 
            &texDesc, D3D12_RESOURCE_STATE_COMMON, 
            &clearValue, IID_PPV_ARGS(frame.gbuffer.targets[i].ReleaseAndGetAddressOf()))))
        {
            return false;
        }

        frame.StateTracker.Register(frame.gbuffer.targets[i].Get(), D3D12_RESOURCE_STATE_COMMON);
    }

    return true;
}

bool DX12_Renderer::CreateGBufferRTVs(UINT frameIndex, FrameResource& fr)
{
    if (!CreateGBuffer(fr, mWidth, mHeight)) return false;

    mGBufferRtvHandles[frameIndex].resize((UINT)GBufferType::Count);

    UINT baseIndex = FrameCount + frameIndex * (UINT)GBufferType::Count;

    for (UINT g = 0; g < (UINT)GBufferType::Count; g++)
    {
        UINT slot = mRtvManager->Allocate();
        D3D12_CPU_DESCRIPTOR_HANDLE handle = mRtvManager->GetCpuHandle(slot);

        mDevice->CreateRenderTargetView(fr.gbuffer.targets[g].Get(), nullptr, handle);
        mGBufferRtvHandles[frameIndex][g] = handle;
    }
    return true;
}

bool DX12_Renderer::CreateDepthStencil(FrameResource& frame, UINT width, UINT height)
{
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D24_UNORM_S8_UINT, width, height, 1, 1);
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

    UINT slot = mDsvManager->Allocate();
    D3D12_CPU_DESCRIPTOR_HANDLE handle = mDsvManager->GetCpuHandle(slot);

    mDevice->CreateDepthStencilView(fr.DepthStencilBuffer.Get(), nullptr, handle);
    mDsvHandles[frameIndex] = handle;

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

void DX12_Renderer::PrepareCommandList()
{
    FrameResource& fr = GetCurrentFrameResource();

    fr.CommandAllocator->Reset();
    mCommandList->Reset(fr.CommandAllocator.Get(), nullptr);
}

void DX12_Renderer::ClearGBuffer()
{
    FrameResource& fr = mFrameResources[mFrameIndex];
    auto& rtvs = mGBufferRtvHandles[mFrameIndex];
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = mDsvHandles[mFrameIndex];

    for (UINT g = 0; g < (UINT)GBufferType::Count; g++) {
        fr.StateTracker.Transition(mCommandList.Get(), fr.gbuffer.targets[g].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    mCommandList->OMSetRenderTargets((UINT)rtvs.size(), rtvs.data(), FALSE, &dsv);

    for (UINT g = 0; g < (UINT)GBufferType::Count; g++) {
        const MRTTargetDesc& cfg = GBUFFER_CONFIG[g];
        mCommandList->ClearRenderTargetView(rtvs[g], cfg.clearColor, 0, nullptr);
    }

    mCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    for (UINT g = 0; g < (UINT)GBufferType::Count; g++) {
        fr.StateTracker.Transition(mCommandList.Get(), fr.gbuffer.targets[g].Get(), D3D12_RESOURCE_STATE_COMMON);
    }
}

void DX12_Renderer::ClearBackBuffer(float clear_color[4])
{
    FrameResource& fr = mFrameResources[mFrameIndex];

    fr.StateTracker.Transition(mCommandList.Get(), fr.RenderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = mBackBufferRtvHandles[mFrameIndex];
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = mDsvHandles[mFrameIndex];
    mCommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);


    mCommandList->ClearRenderTargetView(rtv, clear_color, 0, nullptr);
    mCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
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
    
    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
}

void DX12_Renderer::Render()
{
    static float clear_color[4] = { 0.45f, 0.55f, 0.60f, 1.00f };

    PrepareCommandList();
    ClearGBuffer();
    ClearBackBuffer(clear_color);

    ID3D12DescriptorHeap* heaps[] = { mResource_Heap_Manager->GetHeap() };
    mCommandList->SetDescriptorHeaps(1, heaps);

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
    mCommandList->OMSetRenderTargets(
        1, &mBackBufferRtvHandles[mFrameIndex], FALSE, &mDsvHandles[mFrameIndex]);

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());

    // Viewport 지원
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    TransitionBackBufferToPresent();
    mCommandList->Close();
    PresentFrame();
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

void DX12_Renderer::Cleanup()
{
    for (UINT i = 0; i < FrameCount; i++)
        WaitForFrame(mFrameResources[i].FenceValue);

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CloseHandle(mFenceEvent);
}

RendererContext DX12_Renderer::GetContext() const
{
    RendererContext ctx = {};
    ctx.device = mDevice.Get();
    ctx.cmdList = mCommandList.Get();
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
