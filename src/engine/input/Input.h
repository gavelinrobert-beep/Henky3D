#pragma once
#include <GLFW/glfw3.h>
#include <array>

namespace Henky3D {

class Input {
public:
    static void Initialize(GLFWwindow* window);
    static void Update();
    
    static bool IsKeyDown(int keyCode);
    static bool IsKeyPressed(int keyCode);
    static bool IsKeyReleased(int keyCode);
    
    static bool IsMouseButtonDown(int button);
    static bool IsMouseButtonPressed(int button);
    
    static float GetMouseX() { return s_MouseX; }
    static float GetMouseY() { return s_MouseY; }
    static float GetMouseDeltaX() { return s_MouseDeltaX; }
    static float GetMouseDeltaY() { return s_MouseDeltaY; }

private:
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    
    static std::array<bool, GLFW_KEY_LAST + 1> s_Keys;
    static std::array<bool, GLFW_KEY_LAST + 1> s_PrevKeys;
    static std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> s_MouseButtons;
    static std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> s_PrevMouseButtons;
    static float s_MouseX, s_MouseY;
    static float s_PrevMouseX, s_PrevMouseY;
    static float s_MouseDeltaX, s_MouseDeltaY;
    static GLFWwindow* s_Window;
};

} // namespace Henky3D
