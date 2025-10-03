#include "GameEngine.h"

void GameEngine::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
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
	mTimer->Tick();
	active_scene = SceneManager::Get().GetActiveScene();


	float deltaTime = mTimer->GetDeltaTime();

	Update_Inputs(deltaTime);
	Update_Fixed(deltaTime);
	Update_Scene(deltaTime);
	Update_Late(deltaTime);


	std::vector<RenderData> renderData_list = active_scene->GetRenderable();
	std::shared_ptr<CameraComponent> mainCam = active_scene->GetActiveCamera();

	if (mainCam)
		mRenderer->Render(renderData_list, mainCam);


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
	{
		//if (LOWORD(wParam) == WA_INACTIVE)
		//	m_GameTimer.Stop();
		//else
		//	m_GameTimer.Start();
		//break;
	}
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

			auto active_scene = SceneManager::Get().GetActiveScene();
			auto mainCam = active_scene->GetActiveCamera();
			if (mainCam)
			{
				mainCam->SetViewport({ 0, 0 }, { newWidth, newHeight });
				mainCam->SetScissorRect({ 0, 0 }, { newWidth, newHeight });
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

			std::shared_ptr<Scene> active_scene = SceneManager::Get().GetActiveScene();
			std::shared_ptr<CameraComponent> mainCam = active_scene->GetActiveCamera();

			if (mainCam)
			{
				mainCam->SetViewport({ 0, 0 }, { mPendingWidth, mPendingHeight });
				mainCam->SetScissorRect({ 0, 0 }, { mPendingWidth, mPendingHeight });
			}
		}
	}
	break;

	}
	return(0);
}
