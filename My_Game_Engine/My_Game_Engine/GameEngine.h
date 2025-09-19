#pragma once
#include "Engine/DX_Graphics/Renderer.h"
#include "Engine/Managers/PhysicsManager.h"
#include "Engine/Managers/RendererManager.h"
#include "Engine/Resource/ResourceManager.h"
#include "Scene_Manager.h"

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

    void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
    void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

    RendererManager* GetRendererManager() { return renderer_manager.get(); }
    PhysicsManager* GetPhysicsManager() { return physics_manager.get(); }
    ResourceManager* GetResourceManager() { return resource_manager.get(); }

    RendererContext Get_RenderContext() const { return mRenderer->Get_RenderContext(); };
    RendererContext Get_UploadContext() const { return mRenderer->Get_UploadContext(); };

private:
    GameEngine() = default;  // private constructor
    ~GameEngine() = default; // optional, private destructor

private:
    std::unique_ptr<DX12_Renderer>   mRenderer;
    std::unique_ptr<PhysicsManager>  physics_manager;
    std::unique_ptr<RendererManager> renderer_manager;
    std::unique_ptr<ResourceManager> resource_manager;
};