#pragma once
#include "DescriptorManager.h"

enum class ResourceType;
class ResourceSystem;
class Scene;
class Object;
class Component;

struct PerformanceData
{
    UINT fps;
    DirectX::XMFLOAT4* ambientColor;
};

class UIManager
{
public:
    UIManager() = default;
    ~UIManager() = default;

    void Initialize(HWND hWnd, ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
        DescriptorManager* heapManager, ResourceSystem* resSystem);
    void Shutdown();

    void Update(float dt);

    void Render(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE gameScreenTexture);


    void UpdatePerformanceData(const PerformanceData& data);
    void SetSelectedObject(Object* obj);

private:
    void UpdateResourceWindow();
    void UpdateSceneData();
    void UpdateShortcuts();

    void DrawPerformanceWindow();
    void DrawResourceWindow();
    void DrawInspectorWindow();
    void DrawHierarchyWindow();

    void DrawResourceList();
    void DrawResourceDetails();
    void FilterResources();
    const char* GetResourceTypeString(ResourceType type);

    void DrawSceneNode(Object* obj);
    void DrawObjectNode(Object* obj);
    void DrawComponentInspector(Component* comp);

    void DrawMeshRendererInspector(Component* comp);
    void DrawSkinnedMeshRendererInspector(Component* comp);
    void DrawAnimationControllerInspector(Component* comp);
    void DrawCameraInspector(Component* comp);
    void DrawLightInspector(Component* comp);
    void DrawRigidbodyInspector(Component* comp);

private:
    DescriptorManager* mHeapManager = nullptr;
    ResourceSystem* mResourceSystem = nullptr;

    UINT mImguiFontSrvSlot = (UINT)-1;

    PerformanceData mPerformanceData = { 0, nullptr };

    UINT mSelectedResourceId = -1;
    char mSearchBuffer[128] = {};
    int mCurrentFilterType = 0;
    std::vector<UINT> mFilteredResourceIds;
    bool mNeedFilterUpdate = false;

    Object* mSelectedObject = nullptr;
};