#include "InputManager.h"

void InputManager::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wParam < 256) mKeyStates[wParam] = true;
        break;

    case WM_KEYUP:
        if (wParam < 256) mKeyStates[wParam] = false;
        break;


    case WM_MOUSEMOVE:
    {
        mMousePos.x = (int)(short)LOWORD(lParam);
        mMousePos.y = (int)(short)HIWORD(lParam);

        mMouseDelta.x = mMousePos.x - mPrevMousePos.x;
        mMouseDelta.y = mMousePos.y - mPrevMousePos.y;

        mPrevMousePos = mMousePos;
    }
    break;

    case WM_MOUSEWHEEL:
        mWheelDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        break;

    case WM_LBUTTONDOWN: mKeyStates[VK_LBUTTON] = true; break;
    case WM_LBUTTONUP:   mKeyStates[VK_LBUTTON] = false; break;
    case WM_RBUTTONDOWN: mKeyStates[VK_RBUTTON] = true; break;
    case WM_RBUTTONUP:   mKeyStates[VK_RBUTTON] = false; break;
    case WM_MBUTTONDOWN: mKeyStates[VK_MBUTTON] = true; break;
    case WM_MBUTTONUP:   mKeyStates[VK_MBUTTON] = false; break;
    }
}

void InputManager::EndFrame()
{

    mWheelDelta = 0;

    mMouseDelta = { 0,0 };
}
