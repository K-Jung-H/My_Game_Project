#pragma once
#include "DescriptorManager.h"
#include "Graphic_Shader.h"
#include "Core/Scene.h"


struct ResourceStateTracker
{
    void Register(ID3D12Resource* resource, D3D12_RESOURCE_STATES initialState);
    void Transition(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* resource, D3D12_RESOURCE_STATES newState);

private:
    std::unordered_map<ID3D12Resource*, D3D12_RESOURCE_STATES> mCurrentStates;
};

//=================================================================

struct MRTTargetDesc
{
    DXGI_FORMAT format;
    FLOAT clearColor[4];
};

constexpr MRTTargetDesc GBUFFER_CONFIG[(UINT)GBufferType::Count] =
{
    { DXGI_FORMAT_R16G16B16A16_FLOAT,     {0.0f, 0.0f, 0.0f, 1.0f} },     // Albedo
    { DXGI_FORMAT_R16G16B16A16_FLOAT,       {0.5f, 0.5f, 1.0f, 0.0f} },     // Normal
    { DXGI_FORMAT_R16G16B16A16_FLOAT,         {0.0f, 0.0f, 0.0f, 0.0f} }      // Material
};

struct GBuffer
{
    ComPtr<ID3D12Resource> targets[(UINT)GBufferType::Count];
    ComPtr<ID3D12Resource> Depth;
};

//=================================================================

struct ObjectCBResource
{
    ComPtr<ID3D12Resource> Buffer; 
    ObjectCBData* MappedObjectCB = nullptr;
    UINT HeadOffset = 0;
    UINT MaxObjects = 0;
};

//=================================================================

struct SceneData
{
    float deltaTime;
    float totalTime;
    UINT frameCount;
    UINT padding0;
};

//=================================================================

struct FrameResource
{
    UINT64 FenceValue = 0;
    ComPtr<ID3D12CommandAllocator> CommandAllocator;

    ComPtr<ID3D12Resource> RenderTarget;
    UINT BackBufferRtvSlot_ID = UINT_MAX;

    ComPtr<ID3D12Resource> DepthStencilBuffer;
    UINT DsvSlot_ID = UINT_MAX;
    UINT DepthBufferSrvSlot_ID = UINT_MAX;

    GBuffer gbuffer;
    std::vector<UINT> GBufferRtvSlot_IDs;
    std::vector<UINT> GBufferSrvSlot_IDs;


    ComPtr<ID3D12Resource> Merge_RenderTargets[2];
    UINT MergeRtvSlot_IDs[2] = { UINT_MAX, UINT_MAX };
    UINT MergeSrvSlot_IDs[2] = { UINT_MAX, UINT_MAX };
    UINT Merge_Base_Index = 0;
    UINT Merge_Target_Index = 1;

    ObjectCBResource ObjectCB;

    ResourceStateTracker StateTracker;
};



struct RendererContext
{
    ID3D12Device* device;
    ID3D12GraphicsCommandList* cmdList;
    DescriptorManager* resourceHeap;
};

class RenderData;
class DrawItem;

class DX12_Renderer
{
    static float clear_color[4];

public:
    bool Initialize(HWND hWnd, UINT width, UINT height);
    bool OnResize(UINT newWidth, UINT newHeight);

    void Render(std::vector<RenderData> renderData_list, std::shared_ptr<CameraComponent> render_camera);
    void Cleanup();

    RendererContext Get_RenderContext() const;
    RendererContext Get_UploadContext() const;
    void BeginUpload();
    void EndUpload();

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

    std::unique_ptr<DescriptorManager> mRtvManager;
    std::unique_ptr<DescriptorManager> mDsvManager;
    std::unique_ptr<DescriptorManager> mResource_Heap_Manager;

    // Frame resources
    static const UINT FrameCount = Engine::Frame_Render_Buffer_Count; // Triple buffering
    UINT   mFrameIndex = 0;

    std::array<FrameResource, FrameCount> mFrameResources;


    UINT64 mFenceValue = 0;
    HANDLE mFenceEvent;

    ComPtr<ID3D12Fence> mFence;
    UINT64 mFrameFenceValues[FrameCount] = {};

private:
    // === Resource Upload 傈侩 Command List ===
    ComPtr<ID3D12CommandAllocator>    mUploadAllocator;
    ComPtr<ID3D12GraphicsCommandList> mUploadCommandList;

    UINT64 mUploadFenceValue = 0;
    bool   mUploadClosed = false;

private:
    //==== RenderData Resource
    ComPtr<ID3D12Resource> mSceneData_CB;
    SceneData* mappedSceneDataCB;

    //==== Render DrawCall Target
    std::vector<DrawItem> mDrawItems;

private:
    // === Initialization steps ===
    bool CreateDeviceAndFactory();
    bool CheckMsaaSupport();
    bool CreateCommandQueue();
    bool CreateSwapChain(HWND hWnd, UINT width, UINT height);
    bool CreateDescriptorHeaps();
    bool CreateCommandList_Upload();

    bool Create_Shader();
    bool Create_SceneCBV();
    bool CreateObjectCB(FrameResource& fr, UINT maxObjects);


    // Frame resource creation
    bool CreateFrameResources();
    bool CreateBackBufferRTV(UINT frameIndex, FrameResource& fr);
    bool CreateCommandAllocator(FrameResource& fr);

    bool CreateGBufferRTVs(UINT frameIndex, FrameResource& fr);
    bool CreateGBufferSRVs(UINT frameIndex, FrameResource& fr);
    bool CreateDSV(UINT frameIndex, FrameResource& fr);

    // Helpers (府家胶 积己)
    bool CreateRTVHeap();
    bool CreateDSVHeap();
    bool CreateResourceHeap();

    bool CreateGBuffer(UINT frameIndex, FrameResource& fr);
    bool CreateDepthStencil(FrameResource& fr, UINT width, UINT height);    
    bool Create_Merge_RenderTargets(UINT frameIndex, FrameResource& fr);

    bool CreateCommandList();
    bool CreateFenceObjects();

    // === Rendering steps ===
    void PrepareCommandList();

    void ClearGBuffer();
    void PrepareGBuffer_RTV();
    void PrepareGBuffer_SRV();


    void ClearBackBuffer(float clear_color[4]);
    void TransitionBackBufferToPresent();
    void PresentFrame();

    void WaitForFrame(UINT64 fenceValue);
    FrameResource& GetCurrentFrameResource();

    void GeometryPass(std::vector<RenderData> renderData_list, std::shared_ptr<CameraComponent> render_camera);
    void CompositePass();
    void PostProcessPass();
    void Blit_BackBufferPass();
    void ImguiPass();

    void SortByRenderType(std::vector<RenderData> renderData_list);
    void Render_Objects(ComPtr<ID3D12GraphicsCommandList> cmdList);

    void Update_SceneCBV();
    void UpdateObjectCBs(const std::vector<RenderData>& renderables);

public:
    ImGui_ImplDX12_InitInfo GetImGuiInitInfo() const;

private:
    ComPtr<ID3D12DescriptorHeap> mImguiSrvHeap;
};


