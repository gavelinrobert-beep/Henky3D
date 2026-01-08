#include "Input.h"

namespace Henky3D {

std::array<bool, GLFW_KEY_LAST + 1> Input::s_Keys = {};
std::array<bool, GLFW_KEY_LAST + 1> Input::s_PrevKeys = {};
std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> Input::s_MouseButtons = {};
std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> Input::s_PrevMouseButtons = {};
float Input::s_MouseX = 0.0f;
float Input::s_MouseY = 0.0f;
float Input::s_PrevMouseX = 0.0f;
float Input::s_PrevMouseY = 0.0f;
float Input::s_MouseDeltaX = 0.0f;
float Input::s_MouseDeltaY = 0.0f;
GLFWwindow* Input::s_Window = nullptr;

void Input::Initialize(GLFWwindow* window) {
    s_Window = window;
    
    // Set input callbacks
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    
    // Get initial cursor position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    s_MouseX = static_cast<float>(xpos);
    s_MouseY = static_cast<float>(ypos);
    s_PrevMouseX = s_MouseX;
    s_PrevMouseY = s_MouseY;
}

void Input::Update() {
    s_PrevKeys = s_Keys;
    s_PrevMouseButtons = s_MouseButtons;
    
    // Update mouse delta
    s_MouseDeltaX = s_MouseX - s_PrevMouseX;
    s_MouseDeltaY = s_MouseY - s_PrevMouseY;
    s_PrevMouseX = s_MouseX;
    s_PrevMouseY = s_MouseY;
}

bool Input::IsKeyDown(int keyCode) {
    // Map common ASCII keys to GLFW keys
    if (keyCode >= 'A' && keyCode <= 'Z') {
        return s_Keys[GLFW_KEY_A + (keyCode - 'A')];
    }
    if (keyCode >= '0' && keyCode <= '9') {
        return s_Keys[GLFW_KEY_0 + (keyCode - '0')];
    }
    return s_Keys[keyCode];
}

bool Input::IsKeyPressed(int keyCode) {
    // Map common ASCII keys to GLFW keys
    int glfwKey = keyCode;
    if (keyCode >= 'A' && keyCode <= 'Z') {
        glfwKey = GLFW_KEY_A + (keyCode - 'A');
    } else if (keyCode >= '0' && keyCode <= '9') {
        glfwKey = GLFW_KEY_0 + (keyCode - '0');
    }
    return s_Keys[glfwKey] && !s_PrevKeys[glfwKey];
}

bool Input::IsKeyReleased(int keyCode) {
    int glfwKey = keyCode;
    if (keyCode >= 'A' && keyCode <= 'Z') {
        glfwKey = GLFW_KEY_A + (keyCode - 'A');
    } else if (keyCode >= '0' && keyCode <= '9') {
        glfwKey = GLFW_KEY_0 + (keyCode - '0');
    }
    return !s_Keys[glfwKey] && s_PrevKeys[glfwKey];
}

bool Input::IsMouseButtonDown(int button) {
    return s_MouseButtons[button];
}

bool Input::IsMouseButtonPressed(int button) {
    return s_MouseButtons[button] && !s_PrevMouseButtons[button];
}

void Input::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key >= 0 && key <= GLFW_KEY_LAST) {
        if (action == GLFW_PRESS) {
            s_Keys[key] = true;
        } else if (action == GLFW_RELEASE) {
            s_Keys[key] = false;
        }
    }
}

void Input::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST) {
        if (action == GLFW_PRESS) {
            s_MouseButtons[button] = true;
        } else if (action == GLFW_RELEASE) {
            s_MouseButtons[button] = false;
        }
    }
}

void Input::CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    s_MouseX = static_cast<float>(xpos);
    s_MouseY = static_cast<float>(ypos);
}

} // namespace Henky3D
