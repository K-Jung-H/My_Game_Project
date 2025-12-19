#pragma once
#include "DescriptorManager.h"
#include "DynamicBufferAllocator.h"
#include "Graphic_Shader.h"
#include "Core/Scene.h"
#include "Inspector_UI.h"

// =================================================================
// Resource State Tracker
// =================================================================
struct ResourceStateTracker
{
    void Register(ID3D12Resource* resource, D3D12_RESOURCE_STATES initialState);
    void Transition(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* resource, D3D12_RESOURCE_STATES newState);
    void UAVBarrier(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* resource);

    void Clear() { mCurrentStates.clear(); }
private:
    std::unordered_map<ID3D12Resource*, D3D12_RESOURCE_STATES> mCurrentStates;
};

// =================================================================
// G-Buffer Configuration
// =================================================================
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

// =================================================================
// GPU Data Structures
// =================================================================

struct ClusterLightMeta
{
    UINT offset;
    UINT count;
    XMFLOAT2 padding0;;
};

struct ShadowMatrixData
{
    XMFLOAT4X4 ViewProj;
};

struct LightResource
{
    ComPtr<ID3D12Resource> ClusterBuffer;
    UINT ClusterBuffer_SRV_Index;
    UINT ClusterBuffer_UAV_Index;

    ComPtr<ID3D12Resource> LightBuffer;
    UINT LightBuffer_SRV_Index;

    ComPtr<ID3D12Resource> ClusterLightMetaBuffer;
    UINT ClusterLightMetaBuffer_SRV_Index;
    UINT ClusterLightMetaBuffer_UAV_Index;

    ComPtr<ID3D12Resource> ClusterLightIndicesBuffer;
    UINT ClusterLightIndicesBuffer_SRV_Index;
    UINT ClusterLightIndicesBuffer_UAV_Index;

    ComPtr<ID3D12Resource> GlobalOffsetCounterBuffer;
    UINT GlobalOffsetCounterBuffer_UAV_Index;

    UINT NumLights = 0;

    // Shadow Resources
    ComPtr<ID3D12Resource> SpotShadowArray;      // Texture2DArray
    ComPtr<ID3D12Resource> CsmShadowArray;       // Texture2DArray
    ComPtr<ID3D12Resource> PointShadowCubeArray; // TextureCubeArray

    UINT SpotShadowArray_SRV = UINT_MAX;
    UINT CsmShadowArray_SRV = UINT_MAX;
    UINT PointShadowCubeArray_SRV = UINT_MAX;

    std::vector<UINT> SpotShadow_DSVs;
    std::vector<UINT> CsmShadow_DSVs;
    std::vector<UINT> PointShadow_DSVs;

    D3D12_GPU_VIRTUAL_ADDRESS CurrentShadowMatrixGPUAddress = 0;

    std::unordered_map<LightComponent*, UINT> mLightShadowIndexMap;
    std::vector<LightComponent*> mFrameShadowCastingCSM;
    std::vector<LightComponent*> mFrameShadowCastingSpot;
    std::vector<LightComponent*> mFrameShadowCastingPoint;
};

// =================================================================
// Render Settings & Frame Data
// =================================================================
enum RenderFlags : UINT
{
    RENDER_DEBUG_DEFAULT = 1 << 0,
    RENDER_DEBUG_ALBEDO = 1 << 1,
    RENDER_DEBUG_NORMAL = 1 << 2,
    RENDER_DEBUG_MATERIAL_ROUGHNESS = 1 << 3,
    RENDER_DEBUG_MATERIAL_METALLIC = 1 << 4,
    RENDER_DEBUG_DEPTH_SCREEN = 1 << 5,
    RENDER_DEBUG_DEPTH_VIEW = 1 << 6,
    RENDER_DEBUG_DEPTH_WORLD = 1 << 7,
    RENDER_DEBUG_CLUSTER_AABB = 1 << 8,
    RENDER_DEBUG_CLUSTER_ID = 1 << 9,
    RENDER_DEBUG_LIGHT_COUNT = 1 << 10,
};

struct SceneData
{
    float deltaTime;
    float totalTime;
    UINT frameCount;
    UINT padding0;

    UINT LightCount;
    UINT ClusterIndexCapacity;
    UINT RenderFlags;
    UINT padding1;

    XMFLOAT4 AmbientColor;
};

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

    std::unique_ptr<DynamicBufferAllocator> DynamicAllocator;
    LightResource light_resource;
    ResourceStateTracker StateTracker;
};

struct RendererContext
{
    ID3D12Device* device;
    ID3D12GraphicsCommandList* cmdList;
    DescriptorManager* resourceHeap;
};

class TerrainComponent;
class RenderData;
class DrawItem;

// =================================================================
// DX12 Renderer Class
// =================================================================
class DX12_Renderer
{
    static float clear_color[4];
    XMFLOAT4 mAmbientColor = { 0.4f, 0.4f, 0.4f, 1.0f };

public:
    // --- Core Lifecycle ---
    bool Initialize(HWND m_hWnd, UINT width, UINT height);
    void Cleanup();
    void FlushCommandQueue();

    // --- Resizing ---
    bool ResizeSwapChain(UINT newWidth, UINT newHeight); // OS Window Resizing
    bool ResizeViewport(UINT newWidth, UINT newHeight);  // Game Viewport Resizing

    // --- Getters ---
    UINT GetRenderWidth() const { return mRenderWidth; }
    UINT GetRenderHeight() const { return mRenderHeight; }

    // --- Main Render Loop ---
    void Update_SceneCBV(SceneData& data);
    void Render(std::shared_ptr<Scene> render_scene);

    // --- Utility & Upload Context ---
    Allocation AllocateDynamicBuffer(size_t sizeInBytes, size_t alignment = 256);
    RendererContext Get_RenderContext() const;
    RendererContext Get_UploadContext() const;
    void BeginUpload();
    void EndUpload();
    bool IsUploadOpen() const { return !mUploadClosed; }

    bool test_value = false;

private:
    // --- Internal Resize Logic ---
    bool ExecuteResizeSwapChain();
    bool ExecuteResizeViewport();

    // --- Resource Creation/Destruction Groups ---
    bool CreatePerFrameBuffers();
    void DestroyPerFrameBuffers();

    bool CreateSwapChainResources();
    void DestroySwapChainResources();

    bool CreateRenderResources();
    void DestroyRenderResources();

    // --- Initialization Helpers ---
    bool CreateDeviceAndFactory();
    bool CheckMsaaSupport();
    bool CreateCommandQueue();
    bool CreateSwapChain(HWND m_hWnd, UINT width, UINT height);
    bool CreateDescriptorHeaps();
    bool CreateCommandList();
    bool CreateFenceObjects();
    bool CreateCommandList_Upload();
    bool Create_Shader();
    bool Create_SceneCBV();

    // --- Helper Functions for Resource Creation ---
    bool CreateRTVHeap();
    bool CreateDSVHeap();
    bool CreateResourceHeap();

    bool CreateCommandAllocator(FrameResource& fr);
    bool CreateBackBufferRTV(UINT frameIndex, FrameResource& fr);

    // Per-Frame Resources
    bool CreateDynamicBufferAllocator(FrameResource& fr);
    bool Create_LightResources(FrameResource& fr, UINT maxLights);
    bool Create_ShadowResources(FrameResource& fr);

    // Resolution Dependent Resources
    bool CreateDSV(FrameResource& fr);
    bool CreateDepthStencil(FrameResource& fr, UINT width, UINT height);
    bool CreateGBuffer(FrameResource& fr);
    bool CreateGBufferRTVs(FrameResource& fr);
    bool CreateGBufferSRVs(FrameResource& fr);
    bool Create_Merge_RenderTargets(FrameResource& fr);

    // --- Rendering Steps ---
    void PrepareCommandList();
    void ClearBackBuffer(float clear_color[4]);
    void TransitionBackBufferToPresent();
    void PresentFrame();
    void WaitForFrame(UINT64 fenceValue);

    FrameResource& GetCurrentFrameResource();

    void ClearGBuffer();
    void PrepareGBuffer_RTV();
    void PrepareGBuffer_SRV();

    // Render Passes
    void SkinningPass();
    void GeometryPass(std::shared_ptr<CameraComponent> render_camera);
    void GeometryTerrainPass(std::shared_ptr<CameraComponent> render_camera, const std::vector<TerrainComponent*>& terrains);
    void LightPass(std::shared_ptr<CameraComponent> render_camera);
    void ShadowPass();
    void CompositePass(std::shared_ptr<CameraComponent> render_camera);
    void PostProcessPass(std::shared_ptr<CameraComponent> render_camera);
    void Blit_BackBufferPass();
    void ImguiPass();

    // Render Helpers
    void Render_Objects(ComPtr<ID3D12GraphicsCommandList> cmdList, UINT objectCBVRootParamIndex, const std::vector<DrawItem>& drawList);
    void UpdateObjectCBs(const std::vector<RenderData>& renderables);
    void UpdateLightAndShadowData(std::shared_ptr<CameraComponent> render_camera, const std::vector<LightComponent*>& light_comp_list);
    void CullObjectsForShadow(LightComponent* light, UINT cascadeIdx);
    void CullObjectsForRender(std::shared_ptr<CameraComponent> camera);
    void Bind_SceneCBV(Shader_Type shader_type, UINT rootParameter);

private:
    bool is_initialized = false;

    // Window Size
    UINT mWidth = 0;
    UINT mHeight = 0;

    // Game Viewport Size
    UINT mRenderWidth = 0;
    UINT mRenderHeight = 0;

    // Resize Request State
    bool mResizeSwapChainRequested = false;
    UINT mPendingSwapChainWidth = 0;
    UINT mPendingSwapChainHeight = 0;
    
    bool mResizeViewportRequested = false;
    UINT mPendingViewportWidth = 0;
    UINT mPendingViewportHeight = 0;

    // DX12 Core Objects
    ComPtr<IDXGIFactory4> mFactory;
    ComPtr<IDXGIAdapter1> mAdapter;
    ComPtr<ID3D12Device>  mDevice;

    UINT mMsaa4xQualityLevels = 0;
    bool mEnableMsaa4x = false;

    ComPtr<ID3D12CommandQueue>        mCommandQueue;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    ComPtr<IDXGISwapChain3>           mSwapChain;

    // Descriptor Heaps
    std::unique_ptr<DescriptorManager> mRtvManager;
    std::unique_ptr<DescriptorManager> mDsvManager;
    std::unique_ptr<DescriptorManager> mResource_Heap_Manager;

    // Frame Resources (Triple Buffering)
    static const UINT FrameCount = Engine::Frame_Render_Buffer_Count;
    UINT   mFrameIndex = 0;
    std::array<FrameResource, FrameCount> mFrameResources;

    // Synchronization
    UINT64 mFenceValue = 0;
    HANDLE mFenceEvent;
    ComPtr<ID3D12Fence> mFence;
    UINT64 mFrameFenceValues[FrameCount] = {};

    // Upload Command Queue
    ComPtr<ID3D12CommandAllocator>    mUploadAllocator;
    ComPtr<ID3D12GraphicsCommandList> mUploadCommandList;
    UINT64 mUploadFenceValue = 0;
    bool   mUploadClosed = false;

    // Scene Constant Buffer
    ComPtr<ID3D12Resource> mSceneData_CB;
    SceneData* mappedSceneDataCB;

    // Draw Items
    std::vector<DrawItem> mDrawItems;
    std::vector<DrawItem> mVisibleItems; // After Culling

    // UI System
    UIManager ui_manager;
};