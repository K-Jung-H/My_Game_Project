#pragma once
#include "Renderer.h"
#include "Engine/Managers/PhysicsManager.h"
#include "Engine/Managers/RendererManager.h"

class GameEngine
{
private:
	std::unique_ptr <DX12_Renderer>mRenderer;
	std::unique_ptr<PhysicsManager> physics_manager;
	std::unique_ptr<RendererManager> renderer_manager;

public:
	void OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	void OnDestroy();

	void FrameAdvance();

	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);


	RendererManager* GetRendererManager() { return renderer_manager.get(); }
	PhysicsManager* GetPhysicsManager() { return physics_manager.get(); }

};

