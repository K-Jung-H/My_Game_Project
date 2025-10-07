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
	renderer_manager = std::make_unique<RendererManager>();
	resource_manager = std::make_unique<ResourceManager>();
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
	mTimer->Tick(mFrame);
	active_scene = SceneManager::Get().GetActiveScene();

	float deltaTime = mTimer->GetDeltaTime();

	Update_Inputs(deltaTime);
	Update_Fixed(deltaTime);
	Update_Scene(deltaTime);
	Update_Late(deltaTime);


	mRenderer->Render(active_scene);


	//if (minimap_Camera)
	//	mRenderer->Render(renderable_list, minimap_Camera);

	InputManager::Get().EndFrame();
}


void GameEngine::OnProcessingInputMessage(HWND m_hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(m_hWnd, nMessageID, wParam, lParam))
		return;

	std::shared_ptr<Scene> active_scene = SceneManager::Get().GetActiveScene();
	InputManager::Get().ProcessMessage(nMessageID, wParam, lParam);


	switch (nMessageID)
	{

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
			//mRenderer->SetDebugView(DebugView::Default);
			break;

		case ID_CAMERA_ALBEDO:
			//mRenderer->SetDebugView(DebugView::Albedo);
			break;

		case ID_CAMERA_NORMAL:
			//mRenderer->SetDebugView(DebugView::Normal);
			break;

		case ID_CAMERA_DEPTH:
			//mRenderer->SetDebugView(DebugView::Depth);
			break;

		case ID_TIMER_START_STOP:
			if (mTimer->GetStopped())
				mTimer->Start();
			else
				mTimer->Stop();
			break;

		case ID_TIMER_SETFRAME:
		{
			mFrame;
		}
		break;

		case ID_ADD_OBJECT:
			//SceneManager::Get().GetActiveScene()->SpawnEmptyObject();
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