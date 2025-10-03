#pragma once
#include "DX_Graphics/Renderer.h"
#include "Managers/RendererManager.h"
#include "Resource/ResourceManager.h"
#include "GameTimer.h"
#include "InputManager.h"
#include "Scene_Manager.h"
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

public:
    void OnCreate(HINSTANCE hInstance, HWND hMainWnd);
    void OnDestroy();

    void FrameAdvance();
    void Update_Inputs(float dt);
    void Update_Fixed(float dt);
    void Update_Scene(float dt);
    void Update_Late(float dt);

    void Tick(float rate) { mTimer->Tick(rate); }

    bool IsInitialized() { return Is_Initialized; }

    void OnProcessingInputMessage(HWND m_hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK OnProcessingWindowMessage(HWND m_hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

    GameTimer* GetTimer() { return mTimer.get(); }
    PhysicsSystem* GetPhysicsSystem() { return m_PhysicsSystem.get(); }
    RendererManager* GetRendererManager() { return renderer_manager.get(); }
    ResourceManager* GetResourceManager() { return resource_manager.get(); }


    RendererContext Get_RenderContext() const { return mRenderer->Get_RenderContext(); };
    RendererContext Get_UploadContext() const { return mRenderer->Get_UploadContext(); };

private:
    GameEngine() = default;  // private constructor
    ~GameEngine() = default; // optional, private destructor

private:
    HWND m_hWnd;

    bool Is_Initialized = false;
    
    std::unique_ptr<PhysicsSystem> m_PhysicsSystem;
//    std::unique_ptr<InputManager> m_Input_manager;
    std::unique_ptr<GameTimer> mTimer;
    std::unique_ptr<DX12_Renderer>   mRenderer;
    std::unique_ptr<RendererManager> renderer_manager;
    std::unique_ptr<ResourceManager> resource_manager;

    std::shared_ptr<Scene> active_scene;

private: // Sync to Win api
    UINT mPendingWidth = 0;
    UINT mPendingHeight = 0;
    bool mResizeRequested = false;
};