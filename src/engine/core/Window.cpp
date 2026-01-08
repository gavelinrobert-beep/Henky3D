#include "Window.h"
#include "../input/Input.h"
#include <stdexcept>
#include <iostream>

// Forward declare ImGui GLFW functions
extern "C" {
    void ImGui_ImplGlfw_InitForOpenGL(GLFWwindow* window, bool install_callbacks);
}

namespace Henky3D {

Window::Window(const std::string& title, uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height), m_Title(title), m_Handle(nullptr) {
    
    // Initialize GLFW
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // Set OpenGL version to 4.6 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    
    // Create window
    m_Handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_Handle) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    // Make OpenGL context current
    glfwMakeContextCurrent(m_Handle);
    
    // Enable vsync
    glfwSwapInterval(1);

    // Set user pointer for callbacks
    glfwSetWindowUserPointer(m_Handle, this);
    
    // Set resize callback
    glfwSetFramebufferSizeCallback(m_Handle, FramebufferSizeCallback);
    
    std::cout << "Window created: " << width << "x" << height << std::endl;
}

Window::~Window() {
    if (m_Handle) {
        glfwDestroyWindow(m_Handle);
    }
    glfwTerminate();
}

bool Window::ProcessMessages() {
    glfwPollEvents();
    return !glfwWindowShouldClose(m_Handle);
}

void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win) {
        win->m_Width = width;
        win->m_Height = height;
        if (win->m_EventCallback) {
            win->m_EventCallback();
        }
    }
}

} // namespace Henky3D
