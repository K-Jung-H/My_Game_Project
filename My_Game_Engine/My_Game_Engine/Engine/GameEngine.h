#pragma once
#include "DX_Graphics/Renderer.h"
#include "Resource/ResourceSystem.h"
#include "InputManager.h"
#include "GameTimer.h"
#include "Scene_Manager.h"
#include "Managers/ObjectManager.h"
#include "PhysicsSystem.h"

class GameEngine
{
public:
    static GameEngine& Get()
    {
        static GameEngine instance;
        return instance;
    }

    GameEngine(const GameEngine&) = delete;
    GameEngine& operator=(const GameEngine&) = delete;
    GameEngine(GameEngine&&) = delete;
    GameEngine& operator=(GameEngine&&) = delete;

private:
    GameEngine() = default;  // private constructor
    ~GameEngine() = default; // optional, private destructor

public:
    void OnCreate(HINSTANCE hInstance, HWND hMainWnd);
    void OnDestroy();

    void FrameAdvance();
    void Update_Inputs(float dt);
    void Update_Fixed(float dt);
    void Update_Scene(float dt);
    void Update_Late();

    void Tick(float rate) { mTimer->Tick(rate); }

    bool IsInitialized() { return Is_Initialized; }

    void OnProcessingInputMessage(HWND m_hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK OnProcessingWindowMessage(HWND m_hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

    GameTimer* GetTimer() { return mTimer.get(); }
    PhysicsSystem* GetPhysicsSystem() { return m_PhysicsSystem.get(); }
    ResourceSystem* GetResourceSystem() { return m_ResourceSystem.get(); }
    ObjectManager* GetObjectManager() { return mObjectmanager.get(); }

    RendererContext Get_RenderContext() const { return mRenderer->Get_RenderContext(); };
    RendererContext Get_UploadContext() const { return mRenderer->Get_UploadContext(); };

    std::shared_ptr<Object> GetSelectedObject() { return mSelectedObject; }
    std::shared_ptr<Scene> GetActiveScene() { return active_scene; }

    void SelectObject(std::shared_ptr<Object> obj) { mSelectedObject = obj; }


private:
    HINSTANCE m_hInstance = nullptr;
    HWND m_hWnd = nullptr;

    bool Is_Initialized = false;
    
    std::unique_ptr<PhysicsSystem> m_PhysicsSystem;
    std::unique_ptr<ResourceSystem> m_ResourceSystem;
    std::unique_ptr<GameTimer> mTimer;
    std::unique_ptr<DX12_Renderer>   mRenderer;

    std::unique_ptr<ObjectManager> mObjectmanager;

    std::shared_ptr<Scene> active_scene;
    std::shared_ptr<Object> mSelectedObject;

    float mFrame = 0.0f; 

    SceneData scene_data {};

private: // Sync to Win api
    UINT mPendingWidth = 0;
    UINT mPendingHeight = 0;
    bool mResizeRequested = false;
};

INT_PTR CALLBACK FrameInputProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
std::string OpenFileDialog(const std::vector<std::pair<std::string, std::string>>& filters);
std::string SaveFileDialog(const std::vector<std::pair<std::string, std::string>>& filters);

