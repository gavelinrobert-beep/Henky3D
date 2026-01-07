#pragma once
#include <Windows.h>
#include <array>

namespace Henky3D {

class Input {
public:
    static void Initialize(HWND hwnd);
    static void Update();
    
    static bool IsKeyDown(int keyCode) { return s_Keys[keyCode]; }
    static bool IsKeyPressed(int keyCode) { return s_Keys[keyCode] && !s_PrevKeys[keyCode]; }
    static bool IsKeyReleased(int keyCode) { return !s_Keys[keyCode] && s_PrevKeys[keyCode]; }
    
    static bool IsMouseButtonDown(int button) { return s_MouseButtons[button]; }
    static bool IsMouseButtonPressed(int button) { return s_MouseButtons[button] && !s_PrevMouseButtons[button]; }
    
    static float GetMouseX() { return s_MouseX; }
    static float GetMouseY() { return s_MouseY; }
    static float GetMouseDeltaX() { return s_MouseDeltaX; }
    static float GetMouseDeltaY() { return s_MouseDeltaY; }
    
    static void ProcessRawInput(LPARAM lParam);

private:
    static std::array<bool, 256> s_Keys;
    static std::array<bool, 256> s_PrevKeys;
    static std::array<bool, 5> s_MouseButtons;
    static std::array<bool, 5> s_PrevMouseButtons;
    static float s_MouseX, s_MouseY;
    static float s_MouseDeltaX, s_MouseDeltaY;
};

} // namespace Henky3D
