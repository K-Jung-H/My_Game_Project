#include "pch.h"
#include "GameEngine.h"

void GameEngine::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
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

	physics_manager = std::make_unique<PhysicsManager>();
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

void GameEngine::FrameAdvance()
{
	// Need Timer
	std::shared_ptr<Scene> active_scene = SceneManager::Get().GetActiveScene();
	active_scene->Check_Inputs();
	active_scene->Fixed_Update(0.0f);
	active_scene->Update(0.0f);


	std::vector<std::shared_ptr<MeshRendererComponent>> renderable_list = active_scene->GetRenderable();
	std::shared_ptr<CameraComponent> mainCam = active_scene->GetActiveCamera();

	if (mainCam)
		mRenderer->Render(renderable_list, mainCam);


	//if (minimap_Camera)
	//	mRenderer->Render(renderable_list, minimap_Camera);

}

void GameEngine::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		break;
	case WM_MOUSEMOVE:
		break;
	default:
		break;
	}
}

void GameEngine::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_KEYUP:
		switch (wParam)
		{
		default:
			break;
		}
		break;
	default:
		break;
	}
}

LRESULT CALLBACK GameEngine::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, nMessageID, wParam, lParam))
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
	case WM_SIZE:
		break;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
		break;
	case WM_MOUSEWHEEL:
		OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_CHAR:
		OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
		break;
	}
	return(0);
}
