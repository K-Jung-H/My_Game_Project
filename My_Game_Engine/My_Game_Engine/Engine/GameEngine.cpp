#include "GameEngine.h"
#include "../Resource.h"

void GameEngine::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hMainWnd;

	mTimer = std::make_unique<GameTimer>();
	m_PhysicsSystem = std::make_unique<PhysicsSystem>();
	m_ResourceSystem = std::make_unique<ResourceSystem>();
	m_ResourceSystem->Initialize("Assets");
	m_AvatarSystem = std::make_unique<AvatarDefinitionManager>();
	m_AvatarSystem->Initialize("Assets/AvatarDefinition");


	CoInitialize(NULL);


	mRenderer = std::make_unique<DX12_Renderer>();
	mRenderer->Initialize(hMainWnd, SCREEN_WIDTH, SCREEN_HEIGHT);

	mRenderer->BeginUpload();
	auto ctx = mRenderer->Get_UploadContext();

	SceneManager::Get().CreateScene("wow");

	mRenderer->EndUpload();

	Is_Initialized = true;
}

void GameEngine::OnDestroy()
{
	mRenderer->Cleanup();
}

void GameEngine::Update_Inputs(float dt)
{
	active_scene->Update_Inputs(dt);
}

void GameEngine::Update_Fixed(float dt)
{
	active_scene->Update_Fixed(dt);
}

void GameEngine::Update_Scene(float dt)
{
	active_scene->Update_Scene(dt);
}

void GameEngine::Update_Late()
{
	active_scene->Update_Late();
}


void GameEngine::FrameAdvance()
{
	auto scene_manager = SceneManager::Get();
	mTimer->Tick(mFrame);
	active_scene = SceneManager::Get().GetActiveScene();

	float deltaTime = mTimer->GetDeltaTime();
	float runTime = mTimer->GetRunTime();

	Update_Inputs(deltaTime);
	Update_Fixed(deltaTime);
	Update_Scene(deltaTime);
	Update_Late();

	scene_data.deltaTime = deltaTime;
	scene_data.totalTime = runTime;
	scene_data.LightCount = (UINT)active_scene->GetLightList().size();
	scene_data.ClusterIndexCapacity = 100;

	mRenderer->Update_SceneCBV(scene_data);

	mRenderer->Render(active_scene);


	//if (minimap_Camera)
	//	mRenderer->Render(renderable_list, minimap_Camera);

	InputManager::Get().EndFrame();
}


void GameEngine::OnProcessingInputMessage(HWND m_hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(m_hWnd, nMessageID, wParam, lParam))
		return;

	InputManager::Get().ProcessMessage(nMessageID, wParam, lParam);


	switch (nMessageID)
	{
		case WM_KEYDOWN:
			switch (wParam)
			{
			case VK_SPACE:
				mRenderer->test_value = !mRenderer->test_value;
			default:
				break;
			}
			break;

	default:
		break;
	}
}

LRESULT CALLBACK GameEngine::OnProcessingWindowMessage(HWND m_hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(m_hWnd, nMessageID, wParam, lParam))
		return true;

	switch (nMessageID)
	{
	case WM_ACTIVATE:
		break;


	case WM_SIZE:
	{
		UINT newWidth = LOWORD(lParam);
		UINT newHeight = HIWORD(lParam);

		if (wParam == SIZE_MINIMIZED) break;
		else if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
		{
			if (mRenderer && Is_Initialized)
				mRenderer->OnResize(newWidth, newHeight);

			if (auto scene = SceneManager::Get().GetActiveScene())
			{
				if (auto cam = scene->GetActiveCamera())
				{
					cam->SetViewport({ 0, 0 }, { newWidth, newHeight });
					cam->SetScissorRect({ 0, 0 }, { newWidth, newHeight });
				}
			}
		}
		else
		{
			mPendingWidth = newWidth;
			mPendingHeight = newHeight;
			mResizeRequested = true;
		}
	}
	break;


	case WM_EXITSIZEMOVE:
	{
		if (mResizeRequested)
		{
			if (mRenderer && Is_Initialized)
				mRenderer->OnResize(mPendingWidth, mPendingHeight);
			mResizeRequested = false;

			if (auto scene = SceneManager::Get().GetActiveScene())
			{
				if (auto cam = scene->GetActiveCamera())
				{
					cam->SetViewport({ 0, 0 }, { mPendingWidth, mPendingHeight });
					cam->SetScissorRect({ 0, 0 }, { mPendingWidth, mPendingHeight });
				}
			}
		}
	}
	break;


	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case IDM_EXIT:
			DestroyWindow(m_hWnd);
			break;

		case ID_CAMERA_DEFAULT:
			scene_data.RenderFlags = RENDER_DEBUG_DEFAULT;
			break;

		case ID_CAMERA_ALBEDO:
			scene_data.RenderFlags = RENDER_DEBUG_ALBEDO;
			break;

		case ID_CAMERA_NORMAL:
			scene_data.RenderFlags = RENDER_DEBUG_NORMAL;
			break;

		case ID_MATERIAL_ROUGHNESS:
			scene_data.RenderFlags = RENDER_DEBUG_MATERIAL_ROUGHNESS;
			break;

		case ID_MATERIAL_METALLIC:
			scene_data.RenderFlags = RENDER_DEBUG_MATERIAL_METALLIC;
			break;

		case ID_DEPTH_SCREEN:
			scene_data.RenderFlags = RENDER_DEBUG_DEPTH_SCREEN;
			break;

		case ID_DEPTH_VIEW:
			scene_data.RenderFlags = RENDER_DEBUG_DEPTH_VIEW;
			break;

		case ID_DEPTH_WORLD:
			scene_data.RenderFlags = RENDER_DEBUG_DEPTH_WORLD;
			break;

		case ID_LIGHT_CLUSTER_AREA:
			scene_data.RenderFlags = RENDER_DEBUG_CLUSTER_AABB;
			break;

		case ID_LIGHT_CLUSTER_ID:
			scene_data.RenderFlags = RENDER_DEBUG_CLUSTER_ID;
			break;

		case ID_LIGHT_LIGHT_COUNT:
			scene_data.RenderFlags = RENDER_DEBUG_LIGHT_COUNT;
			break;

		case ID_TIMER_START_STOP:
			if (mTimer->GetStopped())
				mTimer->Start();
			else
				mTimer->Stop();
			break;

		case ID_TIMER_SETFRAME:
		{
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_FRAME_SET), m_hWnd, FrameInputProc, (LPARAM)&mFrame);

			wchar_t buf[64];
			swprintf_s(buf, L"Frame value set to: %f", mFrame);
			MessageBox(m_hWnd, buf, L"Frame Updated", MB_OK);
			break;
		}

		//case ID_ADD_OBJECT:
		//	//SceneManager::Get().GetActiveScene()->SpawnEmptyObject();
		//	break;


		case ID_SCENE_SAVE:
		{
			if (auto scene = SceneManager::Get().GetActiveScene())
			{
				std::string scene_name = scene->GetAlias();
				SceneManager::Get().SaveScene(scene, scene_name);
			}

			break;
		}
		
		case ID_SCENE_SAVE_AS:
		{
			std::vector<std::pair<std::string, std::string>> filters = {
				{"Scene Files (*.json)", "*.json"},
				{"All Files", "*.*"}
			};

			std::string path = SaveFileDialog(filters);

			if (!path.empty())
			{
				if (auto scene = SceneManager::Get().GetActiveScene())
				{
					SceneManager::Get().SaveScene(scene, path);
				}
			}
		}
		break;

		case ID_SCENE_LOAD:
		{
			std::vector<std::pair<std::string, std::string>> filters = {
				{"Scene Files (*.json)", "*.json"},
				{"All Files", "*.*"}
			};

			std::string path = OpenFileDialog(filters);

			if (!path.empty())
			{
				if (auto scene = SceneManager::Get().LoadScene(path))
				{
					SceneManager::Get().SetActiveScene(scene);
					active_scene = scene;
				}
			}
		}
		break;

		case ID_DEBUG_RESET:
			break;

		default:
			break;
		}
	}
	break;

	default:
		break;
	}

	return 0;
}


INT_PTR CALLBACK FrameInputProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static float* pFrameValue = nullptr;

	switch (message)
	{
	case WM_INITDIALOG:
		pFrameValue = reinterpret_cast<float*>(lParam);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			BOOL success = FALSE;
			int val = GetDlgItemInt(hDlg, IDC_EDIT_FRAME_SET, &success, FALSE);
			if (success && pFrameValue)
				*pFrameValue = val;

			EndDialog(hDlg, IDOK);
			return (INT_PTR)TRUE;
		}
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

std::string OpenFileDialog(const std::vector<std::pair<std::string, std::string>>& filters)
{
	std::string filterStr;
	for (const auto& f : filters)
	{
		filterStr += f.first + '\0' + f.second + '\0';
	}
	filterStr += '\0';

	OPENFILENAMEA ofn;
	CHAR szFile[MAX_PATH] = { 0 };
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = nullptr;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = filterStr.c_str();
	ofn.nFilterIndex = 1;

	std::filesystem::path initDir = std::filesystem::absolute("Assets/Scenes");
	std::string absDir = initDir.string();
	ofn.lpstrInitialDir = absDir.c_str();

	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileNameA(&ofn))
		return std::string(ofn.lpstrFile);


	return "";
}


std::string SaveFileDialog(const std::vector<std::pair<std::string, std::string>>& filters)
{
	std::string filterStr;
	for (const auto& f : filters)
	{
		filterStr += f.first + '\0' + f.second + '\0';
	}
	filterStr += '\0';

	OPENFILENAMEA ofn;
	CHAR szFile[MAX_PATH] = { 0 };
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = nullptr;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = filterStr.c_str();
	ofn.nFilterIndex = 1;

	std::filesystem::path initDir = std::filesystem::absolute("Assets/Scenes");
	std::string absDir = initDir.string();
	ofn.lpstrInitialDir = absDir.c_str();

	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	if (GetSaveFileNameA(&ofn))
	{
		std::string result = ofn.lpstrFile;
		if (result.find(".json") == std::string::npos)
			result += ".json";
		return result;
	}

	return "";
}