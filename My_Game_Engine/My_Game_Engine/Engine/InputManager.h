#pragma once

class InputManager
{
public:
    static InputManager& Get()
    {
        static InputManager instance;
        return instance;
    }

    bool IsKeyDown(UINT key) const;
    bool IsKeyUp(UINT key) const;
    bool IsKeyPressedOnce(UINT key) const;
    bool IsKeyReleasedOnce(UINT key) const;

    void ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void EndFrame();

    POINT GetMousePos() const { return mMousePos; }
    POINT GetMouseDelta() const { return mMouseDelta; }
    int   GetWheelDelta() const { return mWheelDelta; }

private:
    InputManager() = default;
    ~InputManager() = default;

    std::bitset<256> mKeyStates{};
    std::bitset<256> mPrevKeyStates{};

    POINT mMousePos{};
    POINT mPrevMousePos{};
    POINT mMouseDelta{};
    int   mWheelDelta = 0;
};