#pragma once
#include "Resource/ResourceSystem.h"

class Object;
class Component;

class ResourceInspector
{
public:
    void Initialize(ResourceSystem* system);
    void Update();
    void Render();

private:
    void DrawResourceList();
    void DrawResourceDetails();

    const char* GetResourceTypeString(ResourceType type);
    void FilterResources();

private:
    ResourceSystem* mResourceSystem = nullptr;

    UINT mSelectedResourceId = Engine::INVALID_ID;
    int mCurrentFilterType = -1;
    char mSearchBuffer[128] = "";

    std::vector<UINT> mFilteredResourceIds;
    bool mNeedFilterUpdate = true;
};
