#pragma once

class InputManager
{
public:
    static InputManager& Get()
    {
        static InputManager instance;
        return instance;
    }
    
    ~InputManager() = default;

    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    void ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam);


    bool IsKeyDown(unsigned char key) const { return mKeyStates[key]; }
    bool IsKeyUp(unsigned char key) const { return !mKeyStates[key]; }

    POINT GetMousePosition() const { return mMousePos; }
    POINT GetMouseDelta() const { return mMouseDelta; }
    short GetMouseWheelDelta() const { return mWheelDelta; }

    void EndFrame(); 

private:
    InputManager() = default;


private:
    std::array<bool, 256> mKeyStates{}; 
    POINT mMousePos{ 0,0 };
    POINT mPrevMousePos{ 0,0 };
    POINT mMouseDelta{ 0,0 };
    short mWheelDelta = 0;
};
