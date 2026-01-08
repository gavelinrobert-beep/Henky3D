#include "GraphicsDevice.h"
#include "../core/Window.h"
#include <stdexcept>
#include <iostream>

namespace Henky3D {

GraphicsDevice::GraphicsDevice(Window* window)
    : m_Window(window->GetHandle()), m_Width(window->GetWidth()), m_Height(window->GetHeight()) {
    
    InitializeOpenGL();
    
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
}

GraphicsDevice::~GraphicsDevice() {
    // OpenGL cleanup is handled by GLFW
}

void GraphicsDevice::InitializeOpenGL() {
    // Load OpenGL functions with GLAD
    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        throw std::runtime_error("Failed to initialize GLAD");
    }
    
    std::cout << "GLAD loaded OpenGL " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << std::endl;
    
    // Set up OpenGL state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    
    // Enable seamless cubemap filtering
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    
    // Set clear color
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
}

void GraphicsDevice::BeginFrame() {
    // Nothing special needed for OpenGL
}

void GraphicsDevice::EndFrame() {
    // Swap buffers
    glfwSwapBuffers(m_Window);
}

void GraphicsDevice::WaitForGPU() {
    // Force OpenGL to finish all commands
    glFinish();
}

void GraphicsDevice::ResizeBuffers(uint32_t width, uint32_t height) {
    m_Width = width;
    m_Height = height;
    
    // Update viewport
    glViewport(0, 0, width, height);
}

} // namespace Henky3D
