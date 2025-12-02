#pragma once
#include "DescriptorManager.h"

#define PAYLOAD_MESH        "DRAG_RES_MESH"
#define PAYLOAD_MATERIAL    "DRAG_RES_MATERIAL"
#define PAYLOAD_TEXTURE     "DRAG_RES_TEXTURE"
#define PAYLOAD_MODEL       "DRAG_RES_MODEL"
#define PAYLOAD_AVATAR      "DRAG_RES_AVATAR"
#define PAYLOAD_SKELETON    "DRAG_RES_SKELETON"
#define PAYLOAD_CLIP        "DRAG_RES_ANIMCLIP"
#define PAYLOAD_MASK        "DRAG_RES_MASK"

enum class ResourceType;
class ResourceSystem;
class Scene;
class Object;
class Component;

class Game_Resource;
class Mesh;
class Material;
class Texture;
class Model;
class Model_Avatar;
class Skeleton;
class AvatarMask;
class AnimationClip;


struct PerformanceData
{
    UINT fps;
    XMFLOAT4* ambientColor;
};


class UIManager
{
public:
    UIManager() = default;
    ~UIManager() = default;

    // ------------------------------------------------------
    // Lifecycle (Initialization & Shutdown)
    // ------------------------------------------------------
    void Initialize(HWND hWnd, ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
        DescriptorManager* heapManager, ResourceSystem* resSystem);
    void Shutdown();

    // ------------------------------------------------------
    // Main Loop
    // ------------------------------------------------------
    void Update(float dt);
    void Render(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE gameScreenTexture);

    // ------------------------------------------------------
    // External Data Setters
    // ------------------------------------------------------
    void UpdatePerformanceData(const PerformanceData& data);
    void SetSelectedObject(Object* obj);

private:
    // ------------------------------------------------------
    // Main Window Drawers (Called in Render)
    // ------------------------------------------------------
    void DrawPerformanceWindow();
    void DrawResourceWindow();
    void DrawInspectorWindow();
    void DrawHierarchyWindow();

    // ------------------------------------------------------
    // Scene & Hierarchy Logic
    // ------------------------------------------------------
    void UpdateSceneData();
    void UpdateShortcuts();
    void DrawSceneNode(Object* obj);
    void DrawObjectNode(Object* obj);

    // ------------------------------------------------------
    // Component Inspectors (Object Details)
    // ------------------------------------------------------
    void DrawComponentInspector(Component* comp);

    void DrawMeshRendererInspector(Component* comp);
    void DrawSkinnedMeshRendererInspector(Component* comp);
    void DrawAnimationControllerInspector(Component* comp);
    void DrawCameraInspector(Component* comp);
    void DrawLightInspector(Component* comp);
    void DrawRigidbodyInspector(Component* comp);

    // ------------------------------------------------------
    // Resource Browser & Details
    // ------------------------------------------------------
    void UpdateResourceWindow(); // Filter updates
    void DrawResourceList();     // Left Panel
    void DrawResourceDetails();  // Right Panel
    void FilterResources();
    const char* GetResourceTypeString(ResourceType type);
    void DrawSimpleTooltip(Game_Resource* res);

    // Resource Detail Implementation (Overloads)
    void DrawDetailInfo(Mesh* mesh);
    void DrawDetailInfo(Material* mat);
    void DrawDetailInfo(Texture* tex);
    void DrawDetailInfo(Model* model);
    void DrawDetailInfo(Model_Avatar* avatar);
    void DrawDetailInfo(Skeleton* skeleton);
    void DrawDetailInfo(AnimationClip* clip);
    void DrawDetailInfo(AvatarMask* mask);

    // Template Fallback
    template<typename T>
    void DrawDetailInfo(T* resource)
    {
        ImGui::Text("Type: Unknown");
    }

    // Template List Drawer
    template<typename T>
    void DrawTypedList(const char* categoryLabel, const char* typeName, const char* payloadType);

private:
    // Systems
    DescriptorManager* mHeapManager = nullptr;
    ResourceSystem* mResourceSystem = nullptr;

    // Resources
    UINT mImguiFontSrvSlot = (UINT)-1;

    // Data
    PerformanceData mPerformanceData = { 0, nullptr };
    Object* mSelectedObject = nullptr;

    // UI State (Resource Window)
    UINT mSelectedResourceId = -1;
    char mSearchBuffer[128] = {};
    int mCurrentFilterType = 0;
    std::vector<UINT> mFilteredResourceIds;
    bool mNeedFilterUpdate = false;
};