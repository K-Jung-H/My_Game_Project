#include "Inspector_UI.h"
#include "GameEngine.h"
#include "Core/Object.h"
#include "Components/RigidbodyComponent.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void UIManager::Initialize(HWND hWnd, ID3D12Device* device, ID3D12CommandQueue* cmdQueue, DescriptorManager* heapManager, ResourceSystem* resSystem)
{
    mHeapManager = heapManager;
    mResourceSystem = resSystem;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    mImguiFontSrvSlot = mHeapManager->Allocate(HeapRegion::UI);

    ImGui_ImplWin32_Init(hWnd);

    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = device;
    init_info.CommandQueue = cmdQueue;
    init_info.NumFramesInFlight = 3;
    init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    init_info.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    init_info.SrvDescriptorHeap = mHeapManager->GetHeap();
    init_info.LegacySingleSrvCpuDescriptor = mHeapManager->GetCpuHandle(mImguiFontSrvSlot);
    init_info.LegacySingleSrvGpuDescriptor = mHeapManager->GetGpuHandle(mImguiFontSrvSlot);

    ImGui_ImplDX12_Init(&init_info);
}

void UIManager::Shutdown()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::Update(float dt)
{
    UpdateResourceWindow();
    UpdateSceneData();
    UpdateShortcuts();
}


void UIManager::Render(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE gameSceneTexture)
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        static bool first_time = true;
        if (first_time)
        {
            first_time = false;

            ImGui::DockBuilderRemoveNode(dockspace_id);

            ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);

            ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

            ImGuiID dock_main_id = dockspace_id;


            ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);

            ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);

            ImGuiID dock_down_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.3f, nullptr, &dock_main_id);

            ImGui::DockBuilderDockWindow("Game Viewport", dock_main_id);
            ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_left_id);
            ImGui::DockBuilderDockWindow("Performance", dock_right_id);
            ImGui::DockBuilderDockWindow("Inspector", dock_right_id);
            ImGui::DockBuilderDockWindow("Resource Inspector", dock_down_id);

            ImGui::DockBuilderFinish(dockspace_id);
        }
    }
    ImGui::End();

    // [게임 뷰포트 창 그리기]
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("Game Viewport"))
    {
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        ImGui::Image((ImTextureID)gameSceneTexture.ptr, viewportSize);
    }
    ImGui::End();
    ImGui::PopStyleVar();

    // [다른 창들 그리기]
    DrawPerformanceWindow();
    DrawResourceWindow();
    DrawInspectorWindow();
    DrawHierarchyWindow();

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(nullptr, (void*)cmdList);
    }
}

void UIManager::UpdatePerformanceData(const PerformanceData& data)
{
    mPerformanceData = data;
}

void UIManager::SetSelectedObject(Object* obj)
{
    mSelectedObject = obj;
}

void UIManager::UpdateResourceWindow()
{
    if (mNeedFilterUpdate)
    {
        FilterResources();
        mNeedFilterUpdate = false;
    }
}

void UIManager::UpdateSceneData()
{
    if (mSelectedObject)
    {
    }
}

void UIManager::UpdateShortcuts()
{
}

void UIManager::DrawPerformanceWindow()
{
    if (ImGui::Begin("Performance"))
    {
        ImGui::Text("FPS: %u", mPerformanceData.fps);
        ImGui::Separator();

        ImGui::Text("Global Lighting");
        if (mPerformanceData.ambientColor)
        {
            ImGui::ColorEdit3("Ambient Color", (float*)mPerformanceData.ambientColor);
            ImGui::DragFloat("Ambient Intensity", &mPerformanceData.ambientColor->w, 0.01f, 0.0f, 5.0f);
        }
        else
        {
            ImGui::TextDisabled("Lighting Data Not Available");
        }
    }
    ImGui::End();
}

void UIManager::DrawResourceWindow()
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

void UIManager::DrawInspectorWindow()
{
    if (ImGui::Begin("Inspector"))
    {
        if (mSelectedObject)
        {
            DrawObjectNode(mSelectedObject);
        }
        else
        {
            ImGui::Text("No Object Selected");
        }
    }
    ImGui::End();
}

void UIManager::DrawHierarchyWindow()
{
    if (ImGui::Begin("Scene Hierarchy"))
    {
        auto scene = GameEngine::Get().GetActiveScene();

        if (scene)
        {
            const auto& rootObjects = scene->GetRootObjectList();
            for (auto& root : rootObjects)
            {
                DrawSceneNode(root);
            }
        }
        else
        {
            ImGui::Text("No Active Scene");
        }
    }
    ImGui::End();
}

void UIManager::DrawResourceList()
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

void UIManager::DrawResourceDetails()
{
    if (mSelectedResourceId == (UINT)-1)
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
    static UINT lastId = (UINT)-1;
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
            if (srvSlot != (UINT)-1)
            {
                D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = mHeapManager->GetGpuHandle(srvSlot);
                ImGui::Image((ImTextureID)gpuHandle.ptr, ImVec2(128, 128));
            }
            else
            {
                ImGui::TextDisabled("No SRV Handle");
            }
        }
    }
}

void UIManager::FilterResources()
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

const char* UIManager::GetResourceTypeString(ResourceType type)
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

void UIManager::DrawSceneNode(Object* obj)
{
    if (!obj) return;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (obj->GetChildren().empty())
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    if (mSelectedObject == obj)
        flags |= ImGuiTreeNodeFlags_Selected;

    bool opened = ImGui::TreeNodeEx((void*)(intptr_t)obj->GetId(), flags, "%s", obj->GetName().c_str());

    if (ImGui::IsItemClicked())
    {
        GameEngine::Get().SelectObject(obj);
    }

    if (opened && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
    {
        for (auto& child : obj->GetChildren())
        {
            DrawSceneNode(child);
        }
        ImGui::TreePop();
    }
}

void UIManager::DrawObjectNode(Object* obj)
{
    ImGui::Text("Name: %s", obj->GetName().c_str());
    ImGui::Text("ID: %u", obj->GetId());

    ImGui::Separator();

    auto transform = obj->GetTransform();
    if (transform)
    {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            DirectX::XMFLOAT3 pos = transform->GetPosition();
            DirectX::XMFLOAT3 rotEuler = transform->GetRotationEuler();
            DirectX::XMFLOAT3 scale = transform->GetScale();

            bool changed = false;
            changed |= ImGui::DragFloat3("Position", &pos.x, 0.01f);
            changed |= ImGui::DragFloat3("Rotation", &rotEuler.x, 0.1f);
            changed |= ImGui::DragFloat3("Scale", &scale.x, 0.01f);

            if (changed)
            {
                transform->SetPosition(pos);
                transform->SetRotationEuler(rotEuler);
                transform->SetScale(scale);
            }
        }
    }

    ImGui::Separator();
    ImGui::Text("Components:");

    const auto& componentMap = obj->GetAllComponents();

    for (const auto& pair : componentMap)
    {
        Component_Type type = pair.first;
        const auto& componentList = pair.second;

        if (type == Component_Type::Transform)
            continue;

        for (const auto& comp : componentList)
        {
            if (comp)
            {
                DrawComponentInspector(comp.get());
            }
        }
    }
}

void UIManager::DrawComponentInspector(Component* comp)
{
    if (!comp) return;

    if (auto mr = dynamic_cast<MeshRendererComponent*>(comp))
    {
        DrawMeshRendererInspector(mr);
    }
    else if (auto smr = dynamic_cast<SkinnedMeshRendererComponent*>(comp))
    {
        DrawSkinnedMeshRendererInspector(smr);
    }
    else if (auto ac = dynamic_cast<AnimationControllerComponent*>(comp))
    {
        DrawAnimationControllerInspector(ac);
    }
    else if (auto cam = dynamic_cast<CameraComponent*>(comp))
    {
        DrawCameraInspector(cam);
    }
    else if (auto light = dynamic_cast<LightComponent*>(comp))
    {
        DrawLightInspector(light);
    }
    else if (auto rb = dynamic_cast<RigidbodyComponent*>(comp))
    {
        DrawRigidbodyInspector(rb);
    }
    else
    {
        if (ImGui::CollapsingHeader("Unknown Component"))
        {
            ImGui::Text("No inspector available.");
        }
    }
}

void UIManager::DrawMeshRendererInspector(Component* comp)
{
    auto mr = static_cast<MeshRendererComponent*>(comp);
    if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto mesh = mr->GetMesh();
        if (mesh)
        {
            ImGui::Text("Mesh: %s (ID: %u)", mesh->GetAlias().c_str(), mesh->GetId());
        }
        else
        {
            ImGui::Text("Mesh: None");
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_ID"))
            {
                UINT id = *(const UINT*)payload->Data;
                auto res = mResourceSystem->GetById<Mesh>(id);
                if (res) mr->SetMesh(id);
            }
            ImGui::EndDragDropTarget();
        }
    }
}

void UIManager::DrawSkinnedMeshRendererInspector(Component* comp)
{
    auto smr = static_cast<SkinnedMeshRendererComponent*>(comp);
    if (ImGui::CollapsingHeader("Skinned Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto mesh = smr->GetMesh();
        if (mesh)
        {
            ImGui::Text("Mesh: %s", mesh->GetAlias().c_str());
        }
        else
        {
            ImGui::Text("Mesh: None");
        }
        ImGui::Text("Buffers Ready: %s", smr->HasValidBuffers() ? "Yes" : "No");
    }
}

void UIManager::DrawAnimationControllerInspector(Component* comp)
{
    auto animCtrl = static_cast<AnimationControllerComponent*>(comp);

    if (animCtrl)
    {
        ImGui::PushID(animCtrl);
        if (ImGui::CollapsingHeader("Animation Controller", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Status: %s",
                animCtrl->IsReady() ? "Ready" : "Not Ready");

            ImGui::Separator();
            ImGui::Indent();

            ResourceSystem* rs = GameEngine::Get().GetResourceSystem();

            if (ImGui::TreeNodeEx("Global Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto currentSkeleton = animCtrl->GetSkeleton();
                std::string skelName = currentSkeleton ? currentSkeleton->GetAlias() : "None";

                if (ImGui::BeginCombo("Skeleton", skelName.c_str()))
                {
                    auto skeletons = rs->GetAllResources<Skeleton>();
                    for (const auto& skel : skeletons)
                    {
                        bool isSelected = (currentSkeleton == skel);
                        ImGui::PushID(skel.get());
                        if (ImGui::Selectable(skel->GetAlias().c_str(), isSelected))
                            animCtrl->SetSkeleton(skel);
                        if (isSelected) ImGui::SetItemDefaultFocus();
                        ImGui::PopID();
                    }
                    ImGui::EndCombo();
                }

                auto currentAvatar = animCtrl->GetModelAvatar();
                std::string avatarName = currentAvatar ? currentAvatar->GetAlias() : "None";

                if (ImGui::BeginCombo("Avatar", avatarName.c_str()))
                {
                    auto avatars = rs->GetAllResources<Model_Avatar>();
                    for (const auto& avatar : avatars)
                    {
                        bool isSelected = (currentAvatar == avatar);
                        ImGui::PushID(avatar.get());
                        if (ImGui::Selectable(avatar->GetAlias().c_str(), isSelected))
                            animCtrl->SetModelAvatar(avatar);
                        if (isSelected) ImGui::SetItemDefaultFocus();
                        ImGui::PopID();
                    }
                    ImGui::EndCombo();
                }

                int layerCount = animCtrl->GetLayerCount();
                if (ImGui::InputInt("Layer Count", &layerCount))
                {
                    if (layerCount < 1) layerCount = 1;
                    animCtrl->SetLayerCount(layerCount);
                }

                ImGui::TreePop();
            }

            ImGui::Separator();
            ImGui::Text("Global Timeline Control");

            bool isPaused = animCtrl->IsPaused();
            if (ImGui::Checkbox("Pause All", &isPaused))
            {
                animCtrl->SetPause(isPaused);
            }

            static float globalProgress = 0.0f;

            if (ImGui::SliderFloat("Global Progress", &globalProgress, 0.0f, 1.0f, "%.2f%%"))
            {
                if (!isPaused)
                {
                    animCtrl->SetPause(true);
                    isPaused = true;
                }

                int count = animCtrl->GetLayerCount();
                for (int i = 0; i < count; ++i)
                {
                    animCtrl->SetLayerNormalizedTime(i, globalProgress);
                }
            }

            ImGui::Separator();
            ImGui::Text("Animation Layers");

            int layerCount = animCtrl->GetLayerCount();
            auto clips = rs->GetAllResources<AnimationClip>();

            for (int i = 0; i < layerCount; ++i)
            {
                ImGui::PushID(i);

                float childHeight = 230.0f;
                if (ImGui::BeginChild("LayerFrame", ImVec2(0, childHeight), true, ImGuiWindowFlags_None))
                {
                    std::string headerName = "Layer " + std::to_string(i);
                    if (i == 0) headerName += " (Base)";
                    else headerName += " (Overlay)";

                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", headerName.c_str());
                    ImGui::SameLine();
                    ImGui::TextDisabled("(Index: %d)", i);
                    ImGui::Separator();

                    float weight = animCtrl->GetLayerWeight(i);
                    if (ImGui::SliderFloat("Layer Weight", &weight, 0.0f, 1.0f))
                    {
                        animCtrl->SetLayerWeight(i, weight);
                    }

                    auto currentMask = animCtrl->GetLayerMask(i);
                    std::string maskName = currentMask ? currentMask->GetAlias() : "None (Full Body)";

                    if (ImGui::BeginCombo("Layer Mask", maskName.c_str()))
                    {
                        bool isNoneSelected = (currentMask == nullptr);
                        if (ImGui::Selectable("None (Full Body)", isNoneSelected))
                        {
                            animCtrl->SetLayerMask(i, nullptr);
                        }
                        if (isNoneSelected) ImGui::SetItemDefaultFocus();

                        const auto& masks = rs->GetAvatarMasks();
                        for (const auto& maskRes : masks)
                        {
                            bool isSelected = (currentMask == maskRes);
                            if (ImGui::Selectable(maskRes->GetAlias().c_str(), isSelected))
                            {
                                animCtrl->SetLayerMask(i, maskRes);
                            }
                            if (isSelected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::Spacing();

                    const char* modeNames[] = { "Loop", "Once", "PingPong" };
                    int currentModeIdx = (int)animCtrl->GetPlaybackMode(i);

                    if (ImGui::Combo("Mode", &currentModeIdx, modeNames, IM_ARRAYSIZE(modeNames)))
                    {
                        animCtrl->SetPlaybackMode((PlaybackMode)currentModeIdx, i);
                    }

                    static float blendTimeInput = 0.2f;
                    ImGui::DragFloat("Blend Time (s)", &blendTimeInput, 0.01f, 0.0f, 5.0f);

                    auto currentClip = animCtrl->GetCurrentClip(i);
                    std::string previewValue = currentClip ? currentClip->GetAlias() : "Select Clip...";
                    std::unordered_map<std::string, int> nameCounts;

                    if (ImGui::BeginCombo("Clip List", previewValue.c_str()))
                    {
                        for (auto& clip : clips)
                        {
                            std::string alias = clip->GetAlias();
                            nameCounts[alias]++;
                            std::string displayName = alias;
                            if (nameCounts[alias] > 1)
                                displayName += " (" + std::to_string(nameCounts[alias]) + ")";

                            bool isSelected = (currentClip == clip);
                            ImGui::PushID(clip.get());

                            if (ImGui::Selectable(displayName.c_str(), isSelected))
                            {
                                animCtrl->Play(i, clip, blendTimeInput, (PlaybackMode)currentModeIdx, animCtrl->GetSpeed(i));
                            }

                            if (isSelected) ImGui::SetItemDefaultFocus();
                            ImGui::PopID();
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Replay"))
                    {
                        if (currentClip)
                            animCtrl->Play(i, currentClip, 0.0f, (PlaybackMode)currentModeIdx, animCtrl->GetSpeed(i));
                    }

                    float speed = animCtrl->GetSpeed(i);
                    if (ImGui::DragFloat("Speed", &speed, 0.01f, 0.0f, 5.0f))
                    {
                        animCtrl->SetSpeed(speed, i);
                    }

                    ImGui::Spacing();

                    float currentProgress = animCtrl->GetLayerNormalizedTime(i);
                    float duration = animCtrl->GetLayerDuration(i);
                    float currentTime = currentProgress * duration;

                    char timeLabel[64];
                    if (duration > 0.0f)
                        sprintf_s(timeLabel, "%.2fs / %.2fs", currentTime, duration);
                    else
                        sprintf_s(timeLabel, "No Clip");

                    if (ImGui::SliderFloat("Seek", &currentProgress, 0.0f, 1.0f, timeLabel))
                    {
                        animCtrl->SetPause(true);
                        animCtrl->SetLayerNormalizedTime(i, currentProgress);
                    }

                    if (animCtrl->IsLayerTransitioning(i))
                    {
                        float progress = animCtrl->GetLayerTransitionProgress(i);
                        char overlay[32];
                        sprintf_s(overlay, "Blending... %.1f%%", progress * 100.0f);

                        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
                        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), overlay);
                        ImGui::PopStyleColor();
                    }
                }
                ImGui::EndChild();

                ImGui::Spacing();
                ImGui::PopID();
            }

            ImGui::Unindent();
        }

        ImGui::PopID();
    }
}

void UIManager::DrawCameraInspector(Component* comp)
{
    auto cam = static_cast<CameraComponent*>(comp);
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        float fov = cam->GetFovY();
        float nearZ = cam->GetNearZ();
        float farZ = cam->GetFarZ();

        if (ImGui::DragFloat("FOV", &fov, 0.1f, 1.0f, 179.0f)) cam->SetFovY(fov);
        if (ImGui::DragFloat("Near Z", &nearZ, 0.1f, 0.01f, 1000.0f)) cam->SetNearZ(nearZ);
        if (ImGui::DragFloat("Far Z", &farZ, 1.0f, 1.0f, 10000.0f)) cam->SetFarZ(farZ);
    }
}

void UIManager::DrawLightInspector(Component* comp)
{
    auto light = static_cast<LightComponent*>(comp);
    if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        DirectX::XMFLOAT3 color = light->GetColor();
        float intensity = light->GetIntensity();
        float range = light->GetRange();

        if (ImGui::ColorEdit3("Color", &color.x)) light->SetColor(color);
        if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 1000.0f)) light->SetIntensity(intensity);
        if (ImGui::DragFloat("Range", &range, 0.1f, 0.0f, 10000.0f)) light->SetRange(range);
    }
}

void UIManager::DrawRigidbodyInspector(Component* comp)
{
    auto rb = static_cast<RigidbodyComponent*>(comp);
    if (ImGui::CollapsingHeader("Rigidbody", ImGuiTreeNodeFlags_DefaultOpen))
    {
        float mass = rb->GetMass();
        bool useGravity = rb->GetUseGravity();

        if (ImGui::DragFloat("Mass", &mass, 0.1f, 0.0f, 1000.0f)) rb->SetMass(mass);
        if (ImGui::Checkbox("Use Gravity", &useGravity)) rb->SetUseGravity(useGravity);
    }
}