#pragma once
#include <GLFW/glfw3.h>
#include <string>
#include <functional>

namespace Henky3D {

class Window {
public:
    using EventCallback = std::function<void()>;

    Window(const std::string& title, uint32_t width, uint32_t height);
    ~Window();

    bool ProcessMessages();
    void SetEventCallback(EventCallback callback) { m_EventCallback = callback; }

    GLFWwindow* GetHandle() const { return m_Handle; }
    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }

private:
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    
    GLFWwindow* m_Handle;
    uint32_t m_Width;
    uint32_t m_Height;
    std::string m_Title;
    EventCallback m_EventCallback;
};

} // namespace Henky3D
