#include "Inspector_UI.h"
#include "GameEngine.h"
#include "Core/Object.h"
#include "Components/RigidbodyComponent.h"
#include "Components/TerrainComponent.h"
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
        {
            float buttonSize = ImGui::GetFrameHeight();
            float availWidth = ImGui::GetContentRegionAvail().x;

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availWidth - buttonSize);

            if (ImGui::Button("+", ImVec2(buttonSize, buttonSize)))
            {
                OpenLoadResourceDialog();
            }
        }

        static float listWidth = 300.0f;

        ImGui::BeginChild("ResourceList", ImVec2(listWidth, 0), true);
        {
            static int filterIdx = 0;
            const char* filters[] = { "All", "Mesh", "Material", "Texture", "Model", "Animation", "Terrain" }; 

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Filter");
            ImGui::SameLine();

            float comboWidth = ImGui::GetContentRegionAvail().x;
            ImGui::SetNextItemWidth(comboWidth);
            ImGui::Combo("##Filter", &filterIdx, filters, IM_ARRAYSIZE(filters));

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

            if (showAll || filterIdx == 6) DrawTypedList<TerrainResource>("Terrains", "Terrain", PAYLOAD_TERRAIN);

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

            if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            {
                if (ImGui::MenuItem("Create Empty Object"))
                {
                    Object* newObj = scene->GetObjectManager()->CreateObject("New Object");

                    if (newObj)
                    {
                        GameEngine::Get().SelectObject(newObj);
                    }
                }
                ImGui::EndPopup();
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

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Create Child Object"))
        {
            auto scene = GameEngine::Get().GetActiveScene();
            Object* newChild = scene->GetObjectManager()->CreateObject("New Child");
            newChild->SetParent(obj);
            GameEngine::Get().SelectObject(newChild);
        }

        if (ImGui::MenuItem("Delete Object"))
        {
            auto scene = GameEngine::Get().GetActiveScene();
            scene->GetObjectManager()->DestroyObject(obj->GetId());
            if (mSelectedObject == obj) GameEngine::Get().SelectObject(nullptr);
        }

        ImGui::EndPopup();
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
    char nameBuffer[128];
    strcpy_s(nameBuffer, obj->GetName().c_str());
    if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
    {
        GameEngine::Get().GetActiveScene()->GetObjectManager()->SetObjectName(obj, nameBuffer);
    }

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

    ImGui::Separator();

    if (ImGui::Button("Add Component", ImVec2(-1, 0)))
    {
        ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        if (ImGui::MenuItem("Mesh Renderer"))
        {
            obj->AddComponent<MeshRendererComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Skinned Mesh Renderer"))
        {
            obj->AddComponent<SkinnedMeshRendererComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Light"))
        {
            obj->AddComponent<LightComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Camera"))
        {
            obj->AddComponent<CameraComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Rigidbody"))
        {
            obj->AddComponent<RigidbodyComponent>();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::MenuItem("Animation Controller"))
        {
            obj->AddComponent<AnimationControllerComponent>();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void UIManager::DrawComponentInspector(Component* comp)
{
    if (!comp) return;

    ImGui::PushID(comp);

    if (auto smr = dynamic_cast<SkinnedMeshRendererComponent*>(comp))
    {
        DrawSkinnedMeshRendererInspector(smr);
    }
    else if (auto mr = dynamic_cast<MeshRendererComponent*>(comp))
    {
        DrawMeshRendererInspector(mr);
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
    else if (auto terrain = dynamic_cast<TerrainComponent*>(comp))
    {
        DrawTerrainInspector(terrain);
    }
    else
    {
        if (ImGui::CollapsingHeader("Unknown Component"))
        {
            ImGui::Text("No inspector available.");
        }
    }

	ImGui::PopID();
}

void UIManager::DrawMeshRendererInspector(Component* comp)
{
    auto mr = static_cast<MeshRendererComponent*>(comp);
    ResourceSystem* rs = GameEngine::Get().GetResourceSystem();

    if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Mesh Source");

        Mesh* currentMesh = mr->GetMesh().get();
        const auto& meshList = rs->GetMeshes();

        DrawResourcePickUI<Mesh>(
            "##MeshSelect",
            currentMesh,
            meshList,
            PAYLOAD_MESH,
            [mr](Mesh* newMesh) {
                if (newMesh) mr->SetMesh(newMesh->GetId());
                else mr->SetMesh(Engine::INVALID_ID);
            }
        );

        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[Static]");

        if (currentMesh)
        {
            ImGui::Separator();
            ImGui::Text("Materials (SubMeshes: %d)", currentMesh->GetSubMeshCount());
            ImGui::Spacing();

            const auto& matList = rs->GetMaterials();
            int subMeshCount = currentMesh->GetSubMeshCount();

            for (int i = 0; i < subMeshCount; ++i)
            {
                ImGui::PushID(i);

                char label[32];
                sprintf_s(label, "Slot [%d]", i);
                ImGui::Text("%s", label);
                ImGui::SameLine();

                UINT currentMatID = mr->GetMaterial(i);
                if (currentMatID == Engine::INVALID_ID && i < currentMesh->submeshes.size())
                {
                    currentMatID = currentMesh->submeshes[i].materialId;
                }

                auto matRes = rs->GetById<Material>(currentMatID);
                Material* currentMat = matRes.get();

                DrawResourcePickUI<Material>(
                    "##MatSelect",
                    currentMat,
                    matList,
                    PAYLOAD_MATERIAL,
                    [mr, i](Material* newMat) {
                        if (newMat) mr->SetMaterial(i, newMat->GetId());
                        else mr->SetMaterial(i, Engine::INVALID_ID);
                    }
                );

                if (currentMat)
                {
                    ImGui::Indent(10.0f);
                    ImGui::Spacing();

                    ImGui::ColorEdit3("Albedo", &currentMat->albedoColor.x);
                    ImGui::Spacing();

                    auto DrawTextureSlot = [&](const char* mapName, UINT& texID, UINT& texSlot)
                        {
                            ImGui::PushID(mapName);

                            ImGui::AlignTextToFramePadding();
                            ImGui::Bullet();
                            ImGui::Text("%-9s:", mapName);
                            ImGui::SameLine();

                            std::string displayStr = "None";
                            bool hasTexture = (texID != Engine::INVALID_ID);

                            if (hasTexture)
                            {
                                if (auto tex = rs->GetById<Texture>(texID))
                                    displayStr = tex->GetAlias().empty() ? "Texture_" + std::to_string(texID) : tex->GetAlias();
                                else
                                    displayStr = "Missing (ID:" + std::to_string(texID) + ")";
                            }

                            if (!hasTexture) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

                            ImGui::Button(displayStr.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0));

                            if (!hasTexture) ImGui::PopStyleColor();

                            if (ImGui::BeginDragDropTarget())
                            {
                                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PAYLOAD_TEXTURE))
                                {
                                    UINT droppedID = *(const UINT*)payload->Data;
                                    if (auto droppedTex = rs->GetById<Texture>(droppedID))
                                    {
                                        texID = droppedID;
                                        texSlot = droppedTex->GetSlot();
                                    }
                                }
                                ImGui::EndDragDropTarget();
                            }

                            if (ImGui::BeginPopupContextItem())
                            {
                                if (ImGui::MenuItem("Clear Texture"))
                                {
                                    texID = Engine::INVALID_ID;
                                    texSlot = UINT_MAX;
                                }
                                ImGui::EndPopup();
                            }

                            if (hasTexture && ImGui::IsItemHovered())
                            {
                                ImGui::BeginTooltip();
                                ImGui::Text("Asset: %s", displayStr.c_str());
                                ImGui::TextDisabled("ID: %d | Slot: %d", texID, texSlot);
                                ImGui::EndTooltip();
                            }

                            ImGui::PopID();
                        };

                    DrawTextureSlot("Diffuse", currentMat->diffuseTexId, currentMat->diffuseTexSlot);
                    DrawTextureSlot("Normal", currentMat->normalTexId, currentMat->normalTexSlot);
                    DrawTextureSlot("Roughness", currentMat->roughnessTexId, currentMat->roughnessTexSlot);
                    DrawTextureSlot("Metallic", currentMat->metallicTexId, currentMat->metallicTexSlot);

                    ImGui::Unindent(10.0f);
                    ImGui::Spacing();
                }

                ImGui::PopID();
            }
        }
        else
        {
            ImGui::TextDisabled("No Mesh Assigned");
        }
    }
}

void UIManager::DrawSkinnedMeshRendererInspector(Component* comp)
{
    auto smr = static_cast<SkinnedMeshRendererComponent*>(comp);
    ResourceSystem* rs = GameEngine::Get().GetResourceSystem();

    if (ImGui::CollapsingHeader("Skinned Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Mesh Source");

        Mesh* currentMesh = smr->GetMesh().get();
        SkinnedMesh* currentSkinnedMesh = dynamic_cast<SkinnedMesh*>(currentMesh);
        const auto& skinnedMeshList = rs->GetSkinnedMeshes();

        DrawResourcePickUI<SkinnedMesh>(
            "##MeshSelect",
            currentSkinnedMesh,
            skinnedMeshList,
            PAYLOAD_MESH,
            [smr](SkinnedMesh* newMesh) {
                if (newMesh) smr->SetMesh(newMesh->GetId());
                else smr->SetMesh(Engine::INVALID_ID);
            }
        );

        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 1.0f, 1.0f), "[Skinned]");

        if (currentMesh)
        {
            ImGui::Separator();
            ImGui::Text("Materials");
            ImGui::Spacing();

            const auto& matList = rs->GetMaterials();
            int subMeshCount = currentMesh->GetSubMeshCount();

            for (int i = 0; i < subMeshCount; ++i)
            {
                ImGui::PushID(i);

                char label[32];
                sprintf_s(label, "Slot [%d]", i);
                ImGui::Text("%s", label);
                ImGui::SameLine();

                UINT currentMatID = smr->GetMaterial(i);
                if (currentMatID == Engine::INVALID_ID && i < currentMesh->submeshes.size())
                {
                    currentMatID = currentMesh->submeshes[i].materialId;
                }

                auto matRes = rs->GetById<Material>(currentMatID);
                Material* currentMat = matRes.get();

                DrawResourcePickUI<Material>(
                    "##MatSelect",
                    currentMat,
                    matList,
                    PAYLOAD_MATERIAL,
                    [smr, i](Material* newMat) {
                        if (newMat) smr->SetMaterial(i, newMat->GetId());
                        else smr->SetMaterial(i, Engine::INVALID_ID);
                    }
                );

                if (currentMat)
                {
                    ImGui::Indent(10.0f);
                    ImGui::Spacing();

                    ImGui::ColorEdit3("Albedo", &currentMat->albedoColor.x);
                    ImGui::Spacing();

                    auto DrawTextureSlot = [&](const char* mapName, UINT& texID, UINT& texSlot)
                        {
                            ImGui::PushID(mapName);

                            ImGui::AlignTextToFramePadding();
                            ImGui::Bullet();
                            ImGui::Text("%-9s:", mapName);
                            ImGui::SameLine();

                            std::string displayStr = "None";
                            bool hasTexture = (texID != Engine::INVALID_ID);

                            if (hasTexture)
                            {
                                if (auto tex = rs->GetById<Texture>(texID))
                                    displayStr = tex->GetAlias().empty() ? "Texture_" + std::to_string(texID) : tex->GetAlias();
                                else
                                    displayStr = "Missing (ID:" + std::to_string(texID) + ")";
                            }

                            if (!hasTexture) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

                            ImGui::Button(displayStr.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0));

                            if (!hasTexture) ImGui::PopStyleColor();

                            if (ImGui::BeginDragDropTarget())
                            {
                                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PAYLOAD_TEXTURE))
                                {
                                    UINT droppedID = *(const UINT*)payload->Data;
                                    if (auto droppedTex = rs->GetById<Texture>(droppedID))
                                    {
                                        texID = droppedID;
                                        texSlot = droppedTex->GetSlot();
                                    }
                                }
                                ImGui::EndDragDropTarget();
                            }

                            if (ImGui::BeginPopupContextItem())
                            {
                                if (ImGui::MenuItem("Clear Texture"))
                                {
                                    texID = Engine::INVALID_ID;
                                    texSlot = UINT_MAX;
                                }
                                ImGui::EndPopup();
                            }

                            if (hasTexture && ImGui::IsItemHovered())
                            {
                                ImGui::BeginTooltip();
                                ImGui::Text("Asset: %s", displayStr.c_str());
                                ImGui::TextDisabled("ID: %d | Slot: %d", texID, texSlot);
                                ImGui::EndTooltip();
                            }

                            ImGui::PopID();
                        };

                    DrawTextureSlot("Diffuse", currentMat->diffuseTexId, currentMat->diffuseTexSlot);
                    DrawTextureSlot("Normal", currentMat->normalTexId, currentMat->normalTexSlot);
                    DrawTextureSlot("Roughness", currentMat->roughnessTexId, currentMat->roughnessTexSlot);
                    DrawTextureSlot("Metallic", currentMat->metallicTexId, currentMat->metallicTexSlot);

                    ImGui::Unindent(10.0f);
                    ImGui::Spacing();
                }

                ImGui::PopID();
            }
        }
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

    if (light && ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent();

        Light_Type type = light->GetLightType();
        const char* typeNames[] = { "Directional", "Point", "Spot" };
        int currentType = static_cast<int>(type);

        if (ImGui::Combo("Light Type", &currentType, typeNames, IM_ARRAYSIZE(typeNames)))
            light->SetLightType(static_cast<Light_Type>(currentType));

        ImGui::Separator();

        DirectX::XMFLOAT3 color = light->GetColor();
        if (ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&color)))
            light->SetColor(color);

        float intensity = light->GetIntensity();
        if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 1000.0f))
            light->SetIntensity(intensity);

        if (type == Light_Type::Directional || type == Light_Type::Spot)
        {
            DirectX::XMFLOAT3 direction = light->GetDirection();
            if (ImGui::DragFloat3("Direction", reinterpret_cast<float*>(&direction), 0.01f, -1.0f, 1.0f))
            {
                if (direction.x == 0.0f && direction.y == 0.0f && direction.z == 0.0f)
                    direction.y = -1.0f;
                light->SetDirection(direction);
            }
        }

        if (type == Light_Type::Point || type == Light_Type::Spot)
        {
            float range = light->GetRange();
            if (ImGui::DragFloat("Range", &range, 0.1f, 0.1f, 10000.0f))
                light->SetRange(range);
        }

        if (type == Light_Type::Spot)
        {
            float inner = DirectX::XMConvertToDegrees(light->GetInnerAngle());
            if (ImGui::DragFloat("Inner Angle", &inner, 0.5f, 0.0f, 90.0f))
                light->SetInnerAngle(DirectX::XMConvertToRadians(inner));

            float outer = DirectX::XMConvertToDegrees(light->GetOuterAngle());
            if (ImGui::DragFloat("Outer Angle", &outer, 0.5f, 0.0f, 180.0f))
                light->SetOuterAngle(DirectX::XMConvertToRadians(outer));
        }

        ImGui::Spacing();

        if (ImGui::TreeNodeEx("Shadow Settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool castShadow = light->CastsShadow();
            if (ImGui::Checkbox("Cast Shadow", &castShadow))
                light->SetCastShadow(castShadow);

            if (castShadow)
            {
                ShadowMode shadowMode = light->GetShadowMode();
                const char* modeNames[] = { "Dynamic", "Static" };
                int currentModeInt = static_cast<int>(shadowMode);

                if (ImGui::Combo("Shadow Mode", &currentModeInt, modeNames, IM_ARRAYSIZE(modeNames)))
                {
                    light->SetShadowMode(static_cast<ShadowMode>(currentModeInt));
                }

                if (static_cast<ShadowMode>(currentModeInt) == ShadowMode::Static)
                {
                    if (ImGui::Button("Bake New ShadowMap"))
                    {
                        light->ForceShadowMapUpdate();
                    }
                    ImGui::SameLine();
                    ImGui::TextDisabled("(One-time manual bake)");
                }

                ImGui::Separator();

                if (type == Light_Type::Directional)
                {
                    DirectionalShadowMode dirMode = light->GetDirectionalShadowMode();
                    const char* dirModeNames[] = { "Default (StaticGlobal)", "CSM (Camera Dependent)" };
                    int currentDirModeInt = static_cast<int>(dirMode);

                    if (ImGui::Combo("Directional Mode", &currentDirModeInt, dirModeNames, IM_ARRAYSIZE(dirModeNames)))
                    {
                        light->SetDirectionalShadowMode(static_cast<DirectionalShadowMode>(currentDirModeInt));
                    }

                    if (static_cast<DirectionalShadowMode>(currentDirModeInt) == DirectionalShadowMode::Default)
                    {
                        float orthoSize = light->GetStaticOrthoSize();
                        if (ImGui::DragFloat("Static Ortho Size", &orthoSize, 1.0f, 1.0f, 10000.0f))
                        {
                            light->SetStaticOrthoSize(orthoSize);
                        }
                    }

                    ImGui::Separator();

                    float lambda = light->GetCascadeLambda();
                    if (ImGui::SliderFloat("Cascade Lambda", &lambda, 0.0f, 1.0f, "%.2f"))
                        light->SetCascadeLambda(lambda);

                    ImGui::TextDisabled("0.0 = Uniform, 1.0 = Logarithmic");
                    ImGui::Separator();
                }

                float shadowNear = light->GetShadowMapNear();
                if (ImGui::DragFloat("Shadow Near", &shadowNear, 0.1f, 0.01f, light->GetShadowMapFar() - 1.0f))
                    light->SetShadowMapNear(shadowNear);

                float shadowFar = light->GetShadowMapFar();
                if (ImGui::DragFloat("Shadow Far", &shadowFar, 1.0f, light->GetShadowMapNear() + 1.0f, 20000.0f))
                    light->SetShadowMapFar(shadowFar);
            }
            ImGui::TreePop();
        }

        ImGui::Unindent();
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

void UIManager::DrawTerrainInspector(Component* comp)
{
    auto terrain = static_cast<TerrainComponent*>(comp);
    ResourceSystem* rs = GameEngine::Get().GetResourceSystem();

    if (ImGui::CollapsingHeader("Terrain Component", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Terrain Source");

        auto currentTerrainRes = terrain->GetTerrainResource();
        auto terrainList = rs->GetAllResources<TerrainResource>();

        DrawResourcePickUI<TerrainResource>(
            "##TerrainSelect",
            currentTerrainRes.get(),
            terrainList,
            PAYLOAD_TERRAIN,
            [terrain](TerrainResource* newRes) {
                if (newRes) terrain->SetTerrain(newRes->GetId());
                else terrain->SetTerrain(Engine::INVALID_ID);
            }
        );

        ImGui::Spacing();

        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Material");

        auto currentMatID = terrain->GetMaterialID();
        auto currentMat = rs->GetById<Material>(currentMatID);
        auto matList = rs->GetMaterials();

        DrawResourcePickUI<Material>(
            "##TerrainMatSelect",
            currentMat.get(),
            matList,
            PAYLOAD_MATERIAL,
            [terrain](Material* newMat) {
                if (newMat) terrain->SetMaterialID(newMat->GetId());
                else terrain->SetMaterialID(Engine::INVALID_ID);
            }
        );

        ImGui::Separator();

        ImGui::Text("Settings");
        ImGui::Spacing();

        bool settingsChanged = false;
        float width = terrain->GetWidth();
        float depth = terrain->GetDepth();
        float maxHeight = terrain->GetMaxHeight();
        int treeDepth = terrain->GetTreeDepth();

        if (ImGui::DragFloat("Width", &width, 1.0f, 1.0f, 10000.0f))
        {
            terrain->SetTerrain_Width(width);
        }
        if (ImGui::DragFloat("Depth", &depth, 1.0f, 1.0f, 10000.0f))
        {
            terrain->SetTerrain_Depth(depth);
        }
        if (ImGui::DragFloat("Max Height", &maxHeight, 0.5f, 0.1f, 2000.0f))
        {
            terrain->SetTerrain_MaxHeight(maxHeight);
        }
        if (ImGui::SliderInt("Tree Depth", &treeDepth, 1, 10))
        {
            terrain->SetTerrain_TreeDepth(treeDepth);
        }

        ImGui::Separator();

        if (ImGui::TreeNodeEx("Terrain Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BulletText("QuadTree Depth: %d", treeDepth);

            if (currentTerrainRes && currentTerrainRes->GetHeightField())
            {
                const auto* heightField = currentTerrainRes->GetHeightField();

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "[HeightMap Data]");
                ImGui::BulletText("Source Width : %u px", heightField->GetWidthCount());
                ImGui::BulletText("Source Height: %u px", heightField->GetHeightCount());

                float cellX = terrain->GetWidth() / (float)(heightField->GetWidthCount() - 1);
                float cellZ = terrain->GetDepth() / (float)(heightField->GetHeightCount() - 1);
                ImGui::BulletText("Cell Spacing : %.2f x %.2f", cellX, cellZ);
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No HeightMap Loaded");
            }

            ImGui::TreePop();
        }
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
            bool matchPath = entry.metaData->path.find(search) != std::string::npos;
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

void UIManager::OpenLoadResourceDialog()
{
    std::vector<std::pair<std::string, std::string>> fileFilters = {
        {"All Supported (*.fbx;*.obj;*.png;*.jpg;*.json)", "*.fbx;*.obj;*.png;*.jpg;*.dds;*.tga;*.json"},
        {"Models (*.fbx;*.obj)", "*.fbx;*.obj"},
        {"Textures (*.png;*.jpg;*.dds)", "*.png;*.jpg;*.dds;*.tga"},
        {"Materials (*.json)", "*.json"},
        {"All Files", "*.*"}
    };

    std::string path = OpenFileDialog(fileFilters);

    if (!path.empty())
    {
        std::string filename = std::filesystem::path(path).stem().string();

        DX12_Renderer* renderer = GameEngine::Get().GetRenderer();
        renderer->BeginUpload();

        LoadResult result;
        GameEngine::Get().GetResourceSystem()->Load(path, filename, result);


        renderer->EndUpload();

        if (result.modelId != Engine::INVALID_ID)
            g_SelectedResID = result.modelId;
        else if (!result.meshIds.empty())
            g_SelectedResID = result.meshIds[0];
        else if (!result.textureIds.empty())
            g_SelectedResID = result.textureIds[0];
    }
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

template<typename T>
bool UIManager::DrawResourcePickUI(const char* label, T* currentRes, const std::vector<std::shared_ptr<T>>& resources, const char* payloadType, std::function<void(T*)> onSelect)
{
    bool changed = false;
    ResourceSystem* resourceSystem = GameEngine::Get().GetResourceSystem();

    std::string previewName = currentRes ? currentRes->GetAlias() : "None";
    if (previewName.empty() && currentRes) previewName = currentRes->GetPath();

    if (ImGui::BeginCombo(label, previewName.c_str()))
    {
        {
            bool isSelected = (currentRes == nullptr);
            if (ImGui::Selectable("None", isSelected))
            {
                onSelect(nullptr);
                changed = true;
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }

        for (const auto& res : resources)
        {
            if (!res) continue;

            bool isSelected = (currentRes == res.get());
            std::string displayName = res->GetAlias();
            if (displayName.empty()) displayName = "(No Alias)";

            ImGui::PushID((int)res->GetId());
            if (ImGui::Selectable(displayName.c_str(), isSelected))
            {
                onSelect(res.get());
                changed = true;
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType))
        {
            UINT id = *(const UINT*)payload->Data;
            auto res = resourceSystem->GetById<T>(id);
            if (res)
            {
                onSelect(res.get());
                changed = true;
            }
        }
        ImGui::EndDragDropTarget();
    }

    return changed;
}