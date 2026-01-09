#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>

namespace Henky3D {

class Window;

class GraphicsDevice {
public:
    static constexpr int FrameCount = 1; // OpenGL doesn't need double buffering like D3D12
    
    GraphicsDevice(Window* window);
    ~GraphicsDevice();

    void BeginFrame();
    void EndFrame();
    void WaitForGPU(); // No-op for OpenGL
    void ResizeBuffers(uint32_t width, uint32_t height);

    GLFWwindow* GetWindow() const { return m_Window; }
    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }

private:
    void InitializeOpenGL();

    GLFWwindow* m_Window;
    uint32_t m_Width;
    uint32_t m_Height;
};

} // namespace Henky3D
