#ifndef PCH_H
#define PCH_H

#pragma once

#include <SDKDDKVer.h>

//==============================================================
// Core Windows / CRT
//==============================================================
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <wrl.h>
#include <shellapi.h>
#include <tchar.h>

#include <string>
#include <string_view>

#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <random>
#include <optional>
#include <functional>
#include <thread>
#include <mutex>
#include <bitset>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>

//==============================================================
// Networking (if needed)
//==============================================================
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")


//==============================================================
// FBX Reading
//==============================================================
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


//==============================================================
// DirectX 12 Core
//==============================================================
#include <d3d12.h>
#include <directx/d3dx12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>

// Multimedia timing
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// DX12 Libraries
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")

#ifdef _DEBUG
#include <dxgidebug.h>
#include <comdef.h>
#endif

// Imgui
#include "../Editor_Source/imgui.h"
#include "../Editor_Source/ImGuizmo.h"

#include "../Editor_Source/imgui_impl_win32.h"
#include "../Editor_Source/imgui_impl_dx12.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//==============================================================
// External Libraries (e.g., ImGui, DirectXTK12)
//==============================================================
// ImGui headers will be included where needed (imgui.h, imgui_impl_win32.h, imgui_impl_dx12.h)
// DirectXTK12 headers (SpriteFont, CommonStates, etc.) also included in specific modules

//==============================================================
// Namespaces
//==============================================================
using namespace DirectX;
using namespace DirectX::PackedVector;
using Microsoft::WRL::ComPtr;

//==============================================================
// Global Variables
//==============================================================
extern HINSTANCE hInst;

#define SCREEN_WIDTH				1920
#define SCREEN_HEIGHT				1080

#endif //PCH_H
