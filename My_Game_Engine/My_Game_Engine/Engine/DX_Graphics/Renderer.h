#pragma once
#include "DescriptorManager.h"
#include "Graphic_Shader.h"
#include "Core/Scene.h"


struct ResourceStateTracker
{
    void Register(ID3D12Resource* resource, D3D12_RESOURCE_STATES initialState);
    void Transition(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* resource, D3D12_RESOURCE_STATES newState);
    void UAVBarrier(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* resource);

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
    ComPtr<ID3D12Resource> LightUploadBuffer;
    GPULight* MappedLightUploadBuffer = nullptr;
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

    //-----------------------------------------------

    ComPtr<ID3D12Resource> SpotShadowArray;   // Texture2DArray
    ComPtr<ID3D12Resource> CsmShadowArray;    // Texture2DArray
    ComPtr<ID3D12Resource> PointShadowCubeArray; // TextureCubeArray

    UINT SpotShadowArray_SRV = UINT_MAX;
    UINT CsmShadowArray_SRV = UINT_MAX;
    UINT PointShadowCubeArray_SRV = UINT_MAX;

    std::vector<UINT> SpotShadow_DSVs;
    std::vector<UINT> CsmShadow_DSVs;
    std::vector<UINT> PointShadow_DSVs;

    ComPtr<ID3D12Resource> ShadowMatrixBuffer;
    ShadowMatrixData* MappedShadowMatrixBuffer = nullptr;
    UINT ShadowMatrixBuffer_SRV_Index = UINT_MAX; 

    std::unordered_map<LightComponent*, UINT> mLightShadowIndexMap;
    std::vector<LightComponent*> mFrameShadowCastingCSM;
    std::vector<LightComponent*> mFrameShadowCastingSpot;
    std::vector<LightComponent*> mFrameShadowCastingPoint;
};



//=================================================================

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
    float padding1;
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

	LightResource light_resource;

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
    bool Initialize(HWND m_hWnd, UINT width, UINT height);
    bool OnResize(UINT newWidth, UINT newHeight);

    void Update_SceneCBV(const SceneData& data);

    void Render(std::shared_ptr<Scene> render_scene);
    void Cleanup();

    RendererContext Get_RenderContext() const;
    RendererContext Get_UploadContext() const;
    void BeginUpload();
    void EndUpload();

    bool test_value = false;

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
    bool CreateSwapChain(HWND m_hWnd, UINT width, UINT height);
    bool CreateDescriptorHeaps();
    bool CreateCommandList_Upload();

    bool Create_Shader();
    bool Create_SceneCBV();
    bool CreateObjectCB(FrameResource& fr, UINT maxObjects);
    bool Create_LightResources(FrameResource& fr, UINT maxLights);
    bool Create_ShadowResources(FrameResource& fr);
    bool CreateShadowMatrixBuffer(FrameResource& fr);

    // Frame resource creation
    bool CreateFrameResources();
    bool CreateSingleFrameResource(FrameResource& fr, UINT frameIndex);
    void DestroyFrameResources();
    void DestroySingleFrameResource(FrameResource& fr);
    bool CreateBackBufferRTV(UINT frameIndex, FrameResource& fr);
    bool CreateCommandAllocator(FrameResource& fr);

    bool CreateGBufferRTVs(FrameResource& fr);
    bool CreateGBufferSRVs(FrameResource& fr);
    bool CreateDSV(FrameResource& fr);

    // Helpers (府家胶 积己)
    bool CreateRTVHeap();
    bool CreateDSVHeap();
    bool CreateResourceHeap();

    bool CreateGBuffer(FrameResource& fr);
    bool CreateDepthStencil(FrameResource& fr, UINT width, UINT height);    
    bool Create_Merge_RenderTargets(FrameResource& fr);

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

    void GeometryPass(std::shared_ptr<CameraComponent> render_camera);
    void LightPass(std::shared_ptr<CameraComponent> render_camera);
    void ShadowPass();
    void CompositePass(std::shared_ptr<CameraComponent> render_camera);
    void PostProcessPass(std::shared_ptr<CameraComponent> render_camera);
    void Blit_BackBufferPass();
    void ImguiPass();

    void Render_Objects(ComPtr<ID3D12GraphicsCommandList> cmdList, UINT objectCBVRootParamIndex);

    void UpdateObjectCBs(const std::vector<RenderData>& renderables);

    void UpdateLightResources(std::shared_ptr<CameraComponent> render_camera, const std::vector<LightComponent*>& light_comp_list);
    void UpdateShadowResources(std::shared_ptr<CameraComponent> render_camera, const std::vector<LightComponent*>& light_comp_list);

    void Bind_SceneCBV(Shader_Type shader_type, UINT rootParameter);

public:
    ImGui_ImplDX12_InitInfo GetImGuiInitInfo() const;

private:
    ComPtr<ID3D12DescriptorHeap> mImguiSrvHeap;
};


void DrawObjectNode(Object* obj);
void DrawInspector(Object* obj);
void DrawComponentInspector(const std::shared_ptr<Component>& comp);
