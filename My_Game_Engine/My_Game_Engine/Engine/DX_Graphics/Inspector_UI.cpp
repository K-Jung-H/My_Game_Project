#include "Inspector_UI.h"
#include "GameEngine.h" 

void ResourceInspector::Initialize(ResourceSystem* system)
{
    mResourceSystem = system;
}

void ResourceInspector::Update()
{
    if (mNeedFilterUpdate)
    {
        FilterResources();
        mNeedFilterUpdate = false;
    }
}

void ResourceInspector::Render()
{
    if (ImGui::Begin("Resource Inspector"))
    {
        static float listWidth = 300.0f;
        ImGui::BeginChild("ResourceList", ImVec2(listWidth, 0), true);
        DrawResourceList();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("ResourceDetails", ImVec2(0, 0), true);
        DrawResourceDetails();
        ImGui::EndChild();
    }
    ImGui::End();
}

void ResourceInspector::DrawResourceList()
{
    if (ImGui::InputText("Search", mSearchBuffer, sizeof(mSearchBuffer)))
    {
        mNeedFilterUpdate = true;
    }

    const char* types[] = { "All", "Mesh", "Material", "Texture", "Model", "ModelAvatar", "Skeleton", "AnimationClip", "AvatarMask" };
    if (ImGui::Combo("Type", &mCurrentFilterType, types, IM_ARRAYSIZE(types)))
    {
        mNeedFilterUpdate = true;
    }

    ImGui::Separator();

    for (UINT id : mFilteredResourceIds)
    {
        auto res = mResourceSystem->GetById<Game_Resource>(id);
        if (!res) continue;

        std::string label = res->GetAlias().empty() ? std::filesystem::path(res->GetPath()).filename().string() : res->GetAlias();
        std::string itemId = label + "##" + std::to_string(id);

        bool isSelected = (mSelectedResourceId == id);
        if (ImGui::Selectable(itemId.c_str(), isSelected))
        {
            mSelectedResourceId = id;
        }

        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("RESOURCE_ID", &id, sizeof(UINT));
            ImGui::Text("%s", label.c_str());
            ImGui::EndDragDropSource();
        }
    }
}

void ResourceInspector::DrawResourceDetails()
{
    if (mSelectedResourceId == Engine::INVALID_ID)
    {
        ImGui::Text("Select a resource to view details.");
        return;
    }

    auto res = mResourceSystem->GetById<Game_Resource>(mSelectedResourceId);
    if (!res) return;

    ImGui::Text("Resource ID: %d", res->GetId());
    ImGui::Text("GUID: %s", res->GetGUID().c_str());
    ImGui::Text("Type: %s", GetResourceTypeString(res->Get_Type()));

    ImGui::Separator();

    static char aliasBuf[128];
    static UINT lastId = Engine::INVALID_ID;
    if (lastId != mSelectedResourceId) 
    {
        strcpy_s(aliasBuf, res->GetAlias().c_str());
        lastId = mSelectedResourceId;
    }

    if (ImGui::InputText("Alias", aliasBuf, sizeof(aliasBuf), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        res->SetAlias(std::string(aliasBuf));
    }

    ImGui::TextWrapped("Path: %s", res->GetPath().c_str());

    if (res->Get_Type() == ResourceType::Texture)
    {
        auto tex = std::dynamic_pointer_cast<Texture>(res);
        if (tex)
        {
            ImGui::Separator();
            ImGui::Text("Preview:");

            UINT srvSlot = tex->GetSlot();
            if (srvSlot != UINT(-1))
            {
                GameEngine& engine = GameEngine::Get();
                RendererContext ctx = engine.Get_UploadContext();
                if (ctx.resourceHeap)
                {
                    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = ctx.resourceHeap->GetGpuHandle(srvSlot);
                    ImGui::Image((ImTextureID)gpuHandle.ptr, ImVec2(128, 128));
                }
            }
            else
            {
                ImGui::TextDisabled("No SRV Handle");
            }
        }
    }
}

void ResourceInspector::FilterResources()
{
    mFilteredResourceIds.clear();
    const auto& resources = mResourceSystem->GetResourceMap();

    std::string search = mSearchBuffer;

    for (const auto& [id, entry] : resources)
    {

        if (mCurrentFilterType > 0)
        {
            if ((int)entry.resource->Get_Type() != (mCurrentFilterType - 1))
                continue;
        }

        if (!search.empty())
        {
            bool matchAlias = entry.alias.find(search) != std::string::npos;
            bool matchPath = entry.path.find(search) != std::string::npos;
            if (!matchAlias && !matchPath)
                continue;
        }

        mFilteredResourceIds.push_back(id);
    }
}

const char* ResourceInspector::GetResourceTypeString(ResourceType type)
{
    switch (type)
    {
    case ResourceType::Mesh: return "Mesh";
    case ResourceType::Material: return "Material";
    case ResourceType::Texture: return "Texture";
    case ResourceType::Model: return "Model";
    case ResourceType::ModelAvatar: return "ModelAvatar";
    case ResourceType::Skeleton: return "Skeleton";
    case ResourceType::AnimationClip: return "AnimationClip";
    case ResourceType::AvatarMask: return "AvatarMask";
    default: return "Unknown";
    }
}