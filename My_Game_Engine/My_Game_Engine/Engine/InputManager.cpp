#include "InputManager.h"

bool InputManager::IsKeyDown(UINT key) const
{
    if (key < 256)
        return mKeyStates.test(key);
    return false;
}

bool InputManager::IsKeyUp(UINT key) const
{
    if (key < 256)
        return !mKeyStates.test(key);
    return true;
}

bool InputManager::IsKeyPressedOnce(UINT key) const
{
    if (key < 256)
        return mKeyStates.test(key) && !mPrevKeyStates.test(key);
    return false;
}

bool InputManager::IsKeyReleasedOnce(UINT key) const
{
    if (key < 256)
        return !mKeyStates.test(key) && mPrevKeyStates.test(key);
    return false;
}

void InputManager::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wParam < 256) mKeyStates.set(wParam, true);
        break;

    case WM_KEYUP:
        if (wParam < 256) mKeyStates.set(wParam, false);
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

    case WM_LBUTTONDOWN: mKeyStates.set(VK_LBUTTON, true); break;
    case WM_LBUTTONUP:   mKeyStates.set(VK_LBUTTON, false); break;
    case WM_RBUTTONDOWN: mKeyStates.set(VK_RBUTTON, true); break;
    case WM_RBUTTONUP:   mKeyStates.set(VK_RBUTTON, false); break;
    case WM_MBUTTONDOWN: mKeyStates.set(VK_MBUTTON, true); break;
    case WM_MBUTTONUP:   mKeyStates.set(VK_MBUTTON, false); break;
    }
}

void InputManager::EndFrame()
{
    mWheelDelta = 0;
    mMouseDelta = { 0,0 };

    mPrevKeyStates = mKeyStates;
}