#include "Inspector_UI.h"
#include "GameEngine.h"
#include "Core/Object.h"
#include "Components/RigidbodyComponent.h"
#include "Resource/Mesh.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static UINT g_SelectedResID = Engine::INVALID_ID;

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

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("Game Viewport"))
    {
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        DX12_Renderer* renderer = GameEngine::Get().GetRenderer();
        bool resized = false;

        if (renderer)
        {
            UINT currentW = renderer->GetRenderWidth();
            UINT currentH = renderer->GetRenderHeight();

            if ((viewportSize.x > 0 && viewportSize.y > 0) &&
                ((UINT)viewportSize.x != currentW || (UINT)viewportSize.y != currentH))
            {
                renderer->ResizeViewport((UINT)viewportSize.x, (UINT)viewportSize.y);
                resized = true;
            }
        }

        if (resized)
        {
            ImGui::Text("Resizing Viewport...");
        }
        else
        {
            ImGui::Image((ImTextureID)gameSceneTexture.ptr, viewportSize);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();

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
        {
            static int filterIdx = 0;
            const char* filters[] = { "All", "Mesh", "Material", "Texture", "Model", "Animation" };
            ImGui::Combo("Filter", &filterIdx, filters, IM_ARRAYSIZE(filters));
            ImGui::Separator();

            bool showAll = (filterIdx == 0);

            if (showAll || filterIdx == 1) DrawTypedList<Mesh>("Meshes", "Mesh", PAYLOAD_MESH);
            if (showAll || filterIdx == 2) DrawTypedList<Material>("Materials", "Material", PAYLOAD_MATERIAL);
            if (showAll || filterIdx == 3) DrawTypedList<Texture>("Textures", "Texture", PAYLOAD_TEXTURE);
            if (showAll || filterIdx == 4) DrawTypedList<Model>("Models", "Model", PAYLOAD_MODEL);

            if (showAll || filterIdx == 5)
            {
                DrawTypedList<Skeleton>("Skeletons", "Skeleton", PAYLOAD_SKELETON);
                DrawTypedList<AnimationClip>("Animation Clips", "Clip", PAYLOAD_CLIP);
                DrawTypedList<AvatarMask>("Avatar Masks", "Mask", PAYLOAD_MASK);
            }
        }
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

void UIManager::UpdateSceneData()
{
    if (mSelectedObject)
    {
    }
}

void UIManager::UpdateShortcuts()
{
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

void UIManager::UpdateResourceWindow()
{
    if (mNeedFilterUpdate)
    {
        FilterResources();
        mNeedFilterUpdate = false;
    }
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
    if (g_SelectedResID == Engine::INVALID_ID)
    {
        ImGui::TextDisabled("Select a resource to view details.");
        return;
    }

    auto res = GameEngine::Get().GetResourceSystem()->GetById<Game_Resource>(g_SelectedResID);
    if (!res)
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Resource Not Found (ID: %d)", g_SelectedResID);
        return;
    }

    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "[ %s ]", res->GetAlias().c_str());
    ImGui::Text("ID: %d", res->GetId());
    ImGui::TextWrapped("Path: %s", res->GetPath().c_str());
    ImGui::Separator();
    ImGui::Spacing();

    switch (res->Get_Type())
    {
    case ResourceType::Mesh:          DrawDetailInfo(static_cast<Mesh*>(res.get())); break;
    case ResourceType::Material:      DrawDetailInfo(static_cast<Material*>(res.get())); break;
    case ResourceType::Texture:       DrawDetailInfo(static_cast<Texture*>(res.get())); break;
    case ResourceType::Model:         DrawDetailInfo(static_cast<Model*>(res.get())); break;
    case ResourceType::ModelAvatar:   DrawDetailInfo(static_cast<Model_Avatar*>(res.get())); break;
    case ResourceType::Skeleton:      DrawDetailInfo(static_cast<Skeleton*>(res.get())); break;
    case ResourceType::AnimationClip: DrawDetailInfo(static_cast<AnimationClip*>(res.get())); break;
    case ResourceType::AvatarMask:    DrawDetailInfo(static_cast<AvatarMask*>(res.get())); break;
    default: ImGui::Text("Unknown Type Details"); break;
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

void UIManager::DrawSimpleTooltip(Game_Resource* res)
{
    ImGui::Text("Name: %s", res->GetAlias().c_str());
    ImGui::TextDisabled("Path: %s", res->GetPath().c_str());
}

void UIManager::DrawDetailInfo(Mesh* mesh)
{
    ImGui::Text("Vertex Count: %d", mesh->GetVertexCount());
    ImGui::Text("SubMesh Count: %d", mesh->GetSubMeshCount());
    ImGui::Text("Index Count:  %d", mesh->GetIndexCount());

    SkinnedMesh* skinnedMesh = dynamic_cast<SkinnedMesh*>(mesh);
    if (skinnedMesh) ImGui::BulletText("Skinned Mesh (Has Bones)");
}

void UIManager::DrawDetailInfo(Material* mat)
{
    ImGui::ColorEdit3("Albedo Color", &mat->albedoColor.x);
    ImGui::SliderFloat("Roughness", &mat->roughness, 0.0f, 1.0f);
    ImGui::SliderFloat("Metallic", &mat->metallic, 0.0f, 1.0f);

    ImGui::Separator();
    ImGui::Text("Texture Maps");

    ResourceSystem* resSystem = GameEngine::Get().GetResourceSystem();
    RendererContext rendererContext = GameEngine::Get().Get_RenderContext();

    auto DrawTextureSlot = [&](const char* label, UINT texId)
        {
            ImGui::Text("%-12s:", label);
            ImGui::SameLine();

            if (texId == Engine::INVALID_ID)
            {
                ImGui::TextDisabled("None");
                return;
            }

            auto tex = resSystem->GetById<Texture>(texId);
            if (!tex)
            {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Missing (ID: %d)", texId);
                return;
            }

            ImGui::Text("%s (ID: %d)", tex->GetAlias().c_str(), texId);

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Preview");

                UINT slot = tex->GetSlot();
                D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = rendererContext.resourceHeap->GetGpuHandle(slot);

                ImGui::Image((ImTextureID)gpuHandle.ptr, ImVec2(128, 128));

                ImGui::EndTooltip();
            }
        };

    DrawTextureSlot("Diffuse", mat->diffuseTexId);
    DrawTextureSlot("Normal", mat->normalTexId);
    DrawTextureSlot("Roughness", mat->roughnessTexId);
    DrawTextureSlot("Metallic", mat->metallicTexId);
}

void UIManager::DrawDetailInfo(Texture* tex)
{
    ImGui::Text("Resolution: %d x %d", tex->GetWidth(), tex->GetHeight());

    RendererContext rendererContext = GameEngine::Get().Get_RenderContext();

    UINT slot = tex->GetSlot();
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = rendererContext.resourceHeap->GetGpuHandle(slot);

    ImGui::Spacing();
    ImGui::Text("Preview:");

    ImGui::Image((ImTextureID)gpuHandle.ptr, ImVec2(200, 200));
}

void UIManager::DrawDetailInfo(Model* model)
{
    ImGui::Text("Mesh Count:     %d", model->GetMeshCount());
    ImGui::Text("Material Count: %d", model->GetMaterialCount());
    ImGui::Text("Texture Count:  %d", model->GetTextureCount());
}

void UIManager::DrawDetailInfo(Model_Avatar* avatar)
{
    ImGui::Text("Bone Map Size: %d", (int)avatar->GetBoneMap().size());

    const char* typeLabel = "Unknown";
    switch (avatar->GetDefinitionType())
    {
    case DefinitionType::None:     typeLabel = "None"; break;
    case DefinitionType::Humanoid: typeLabel = "Humanoid"; break;
    case DefinitionType::Animal:   typeLabel = "Animal"; break;
    }

    ImGui::Text("Definition Type: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "%s", typeLabel);
}

void UIManager::DrawDetailInfo(Skeleton* skeleton)
{
    ImGui::Text("Bone Count: %d", skeleton->GetBoneCount());

    const auto& names = skeleton->GetBoneNames();

    if (ImGui::CollapsingHeader("Bone Names", ImGuiTreeNodeFlags_None))
    {
        ImGui::BeginChild("BoneList", ImVec2(0, 200), true);
        for (size_t i = 0; i < names.size(); ++i)
        {
            ImGui::Text("[%03d] %s", (int)i, names[i].c_str());
        }
        ImGui::EndChild();
    }
}

void UIManager::DrawDetailInfo(AnimationClip* clip)
{
    ImGui::Text("Duration:  %.2f sec", clip->GetDuration());
    ImGui::Text("FrameRate: %.2f FPS", clip->GetTicksPerSecond());
    ImGui::Text("Total Keyframes: %d", clip->GetTotalKeyframes());
}

void UIManager::DrawDetailInfo(AvatarMask* mask)
{
    const auto& weightMap = mask->GetMetaMap();
    ImGui::Text("Masked Bones Count: %d", (int)weightMap.size());

    if (weightMap.empty()) return;

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Bone Weights", ImGuiTreeNodeFlags_DefaultOpen))
    {
        std::vector<std::pair<std::string, float>> sortedList(weightMap.begin(), weightMap.end());
        std::sort(sortedList.begin(), sortedList.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

        ImGui::BeginChild("WeightScrollRegion", ImVec2(0, 250), true);

        for (const auto& item : sortedList)
        {
            const std::string& name = item.first;
            float weight = item.second;

            ImGui::Text("%s", name.c_str());
            ImGui::SameLine(200);

            ImVec4 color = ImVec4(1.f, 1.f, 1.f, 1.f);
            const char* stateText = "";

            if (weight >= 0.99f)
            {
                color = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
                stateText = "(Include)";
            }
            else if (weight <= 0.01f)
            {
                color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                stateText = "(Exclude)";
            }

            ImGui::TextColored(color, "%.2f %s", weight, stateText);
        }

        ImGui::EndChild();
    }
}

template<typename T>
void UIManager::DrawTypedList(const char* categoryLabel, const char* typeName, const char* payloadType)
{
    ResourceSystem* resourceSystem = GameEngine::Get().GetResourceSystem();
    auto resources = resourceSystem->GetAllResources<T>();

    if (resources.empty()) return;

    if (ImGui::TreeNodeEx(categoryLabel, ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (auto& res : resources)
        {
            ImGui::PushID(res->GetId());

            bool isSelected = (g_SelectedResID == res->GetId());
            if (ImGui::Selectable(res->GetAlias().c_str(), isSelected))
            {
                g_SelectedResID = res->GetId();
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                UINT id = res->GetId();
                ImGui::SetDragDropPayload(payloadType, &id, sizeof(UINT));

                ImGui::Text("%s: %s", typeName, res->GetAlias().c_str());

                if constexpr (std::is_same_v<T, Texture>)
                {
                    ImGui::SameLine();

                    UINT slot = res->GetSlot();
                    if (mHeapManager)
                    {
                        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = mHeapManager->GetGpuHandle(slot);
                        ImGui::Image((ImTextureID)gpuHandle.ptr, ImVec2(32, 32));
                    }
                }

                ImGui::EndDragDropSource();
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                DrawSimpleTooltip(res.get());
                ImGui::EndTooltip();
            }

            ImGui::PopID();
        }
        ImGui::TreePop();
    }
}