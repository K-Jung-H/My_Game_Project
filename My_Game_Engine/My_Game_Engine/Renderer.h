#pragma once
#include "pch.h"

struct MRTTargetDesc
{
    DXGI_FORMAT format;
    FLOAT clearColor[4];
};

enum class GBufferType : UINT
{
    Albedo = 0,
    Normal,
    Material,
    Count
};


constexpr MRTTargetDesc GBUFFER_CONFIG[(UINT)GBufferType::Count] =
{
    { DXGI_FORMAT_R8G8B8A8_UNORM,     {0.0f, 0.0f, 0.0f, 1.0f} },     // Albedo
    { DXGI_FORMAT_R16G16B16A16_FLOAT, {0.5f, 0.5f, 1.0f, 0.0f} },     // Normal
    { DXGI_FORMAT_R8G8_UNORM,         {0.0f, 0.0f, 0.0f, 0.0f} }      // Material
};


struct GBuffer
{
    ComPtr<ID3D12Resource> targets[(UINT)GBufferType::Count];
    ComPtr<ID3D12Resource> Depth;
};


struct ResourceStateTracker 
{
    void Register(ID3D12Resource* resource, D3D12_RESOURCE_STATES initialState);
    void Transition(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* resource, D3D12_RESOURCE_STATES newState);

private:
    std::unordered_map<ID3D12Resource*, D3D12_RESOURCE_STATES> mCurrentStates;
};


struct FrameResource
{
    UINT64 FenceValue = 0;
    ComPtr<ID3D12CommandAllocator> CommandAllocator;
    ComPtr<ID3D12Resource> RenderTarget;
    ComPtr<ID3D12Resource> DepthStencilBuffer;

    GBuffer gbuffer;
    ResourceStateTracker StateTracker;
};


class DX12_Renderer
{
public:
    bool Initialize(HWND hWnd, UINT width, UINT height);
    bool OnResize(UINT newWidth, UINT newHeight);

    void Render();
    void Cleanup();

private:

    UINT mWidth = 0;
    UINT mHeight = 0;


    ComPtr<IDXGIFactory4> mFactory;
    ComPtr<IDXGIAdapter1> mAdapter;
    ComPtr<ID3D12Device>  mDevice;

    // MSAA
    UINT mMsaa4xQualityLevels = 0;
    bool mEnableMsaa4x = false;


    // Core DX12 objects
    ComPtr<ID3D12CommandQueue>     mCommandQueue;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    ComPtr<IDXGISwapChain3>        mSwapChain;

    // RTV Heap
    ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    UINT mRtvDescriptorSize = 0;

    // DSV Heap
    ComPtr<ID3D12DescriptorHeap> mDsvHeap;
    UINT mDsvDescriptorSize = 0;

    // Frame resources
    static const UINT FrameCount = 3; // Triple buffering
    UINT   mFrameIndex = 0;

    std::array<FrameResource, FrameCount> mFrameResources;

    std::array<std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>, FrameCount> mGBufferRtvHandles;
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, FrameCount> mBackBufferRtvHandles;
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, FrameCount> mDsvHandles;

    UINT64 mFenceValue = 0;
    HANDLE mFenceEvent;

    ComPtr<ID3D12Fence> mFence;
    UINT64 mFrameFenceValues[FrameCount] = {};

private:
    // === Initialization steps ===
    bool CreateDeviceAndFactory();
    bool CheckMsaaSupport();
    bool CreateCommandQueue();
    bool CreateSwapChain(HWND hWnd, UINT width, UINT height);
    bool CreateDescriptorHeaps();


    // Frame resource creation
    bool CreateFrameResources();
    bool CreateBackBufferRTV(UINT frameIndex, FrameResource& fr);
    bool CreateCommandAllocator(FrameResource& fr);
    bool CreateGBufferRTVs(UINT frameIndex, FrameResource& fr);
    bool CreateDSV(UINT frameIndex, FrameResource& fr);

    // Helpers (府家胶 积己)
    bool CreateRTVHeap();
    bool CreateDSVHeap();
    bool CreateGBuffer(FrameResource& fr, UINT width, UINT height);
    bool CreateDepthStencil(FrameResource& fr, UINT width, UINT height);    
    
    bool CreateCommandList();
    bool CreateFenceObjects();

    // === Rendering steps ===
    void PrepareCommandList();
    void ClearGBuffer();
    void ClearBackBuffer(float clear_color[4]);
    void TransitionBackBufferToPresent();
    void PresentFrame();

    void WaitForFrame(UINT64 fenceValue);
    FrameResource& GetCurrentFrameResource();


public:
    ImGui_ImplDX12_InitInfo GetImGuiInitInfo() const;

private:
    ComPtr<ID3D12DescriptorHeap> mImguiSrvHeap;
};


