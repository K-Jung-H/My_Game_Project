#include "GameEngine.h"
#include "../Resource.h"

void GameEngine::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hMainWnd;

	Is_Initialized = true;
	CoInitialize(NULL);

	mRenderer = std::make_unique<DX12_Renderer>();
	mRenderer->Initialize(hMainWnd, SCREEN_WIDTH, SCREEN_HEIGHT);

	// ImGui ÃÊ±âÈ­
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.ConfigFlags |= ImGuiDockNodeFlags_PassthruCentralNode;

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	ImGui_ImplWin32_Init(hMainWnd);

	ImGui_ImplDX12_InitInfo init_info = mRenderer->GetImGuiInitInfo();
	ImGui_ImplDX12_Init(&init_info);

	mTimer = std::make_unique<GameTimer>();
	m_PhysicsSystem = std::make_unique<PhysicsSystem>();
	m_ResourceSystem = std::make_unique<ResourceSystem>();
	m_ResourceSystem->Initialize("Assets");
	mObjectmanager = std::make_unique<ObjectManager>();

	mRenderer->BeginUpload();
	auto ctx = mRenderer->Get_UploadContext();

	SceneManager::Get().CreateScene("wow");




	mRenderer->EndUpload();


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

void GameEngine::Update_Late(float dt)
{
	active_scene->Update_Late(dt);
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
	Update_Late(deltaTime);

	scene_data.deltaTime = deltaTime;
	scene_data.totalTime = runTime;

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

//	std::shared_ptr<Scene> active_scene = SceneManager::Get().GetActiveScene();
	InputManager::Get().ProcessMessage(nMessageID, wParam, lParam);


	switch (nMessageID)
	{

		case WM_KEYDOWN:
			switch (wParam)
			{
			case 'P':
			{
				if (auto scene = SceneManager::Get().LoadScene("test_scene.json"))
				{
					SceneManager::Get().SetActiveScene(scene);
					active_scene = scene;
				}
			}
			break;
				
			case 'O':
			{
				if (auto scene = SceneManager::Get().GetActiveScene())
					SceneManager::Get().SaveScene(scene, "test_scene");
			}
			break;

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
			if (mRenderer)
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
			if (mRenderer)
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

//		case ID_SCENE_SAVE:
//		case ID_SCENE_SAVE_AS:
		case ID_ADD_OBJECT:
		{

			if (auto scene = SceneManager::Get().LoadScene("test_scene.json"))
			{
				SceneManager::Get().SetActiveScene(scene);
				active_scene = scene;
			}

			//if (auto scene = SceneManager::Get().GetActiveScene())
			//	SceneManager::Get().SaveScene(scene, "test_scene");

			break;
		}
		
		break;



	//	case ID_SCENE_LOAD:
		{
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
