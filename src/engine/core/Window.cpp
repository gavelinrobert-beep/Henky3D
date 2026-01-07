#include "Window.h"
#include "../input/Input.h"
#include <stdexcept>
#include <windef.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Henky3D {

Window::Window(const std::string& title, uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height), m_Title(title), m_Handle(nullptr) {
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"Henky3DWindowClass";
    
    if (!RegisterClassExW(&wc)) {
        throw std::runtime_error("Failed to register window class");
    }

    RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    std::wstring wTitle(title.begin(), title.end());
    m_Handle = CreateWindowExW(
        0,
        wc.lpszClassName,
        wTitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        wc.hInstance,
        this
    );

    if (!m_Handle) {
        throw std::runtime_error("Failed to create window");
    }

    ShowWindow(m_Handle, SW_SHOW);
    UpdateWindow(m_Handle);
}

Window::~Window() {
    if (m_Handle) {
        DestroyWindow(m_Handle);
    }
}

bool Window::ProcessMessages() {
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;

    Window* window = nullptr;
    if (msg == WM_CREATE) {
        CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = reinterpret_cast<Window*>(createStruct->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else {
        window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            if (window && wParam != SIZE_MINIMIZED) {
                window->m_Width = LOWORD(lParam);
                window->m_Height = HIWORD(lParam);
                if (window->m_EventCallback) {
                    window->m_EventCallback();
                }
            }
            return 0;
        case WM_INPUT:
            Input::ProcessRawInput(lParam);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

} // namespace Henky3D
