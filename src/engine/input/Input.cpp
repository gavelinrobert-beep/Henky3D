#include "Input.h"

namespace Henky3D {

std::array<bool, 256> Input::s_Keys = {};
std::array<bool, 256> Input::s_PrevKeys = {};
std::array<bool, 5> Input::s_MouseButtons = {};
std::array<bool, 5> Input::s_PrevMouseButtons = {};
float Input::s_MouseX = 0.0f;
float Input::s_MouseY = 0.0f;
float Input::s_MouseDeltaX = 0.0f;
float Input::s_MouseDeltaY = 0.0f;

void Input::Initialize(HWND hwnd) {
    RAWINPUTDEVICE rid[2] = {};
    
    // Register mouse
    rid[0].usUsagePage = 0x01;
    rid[0].usUsage = 0x02;
    rid[0].dwFlags = 0;
    rid[0].hwndTarget = hwnd;
    
    // Register keyboard
    rid[1].usUsagePage = 0x01;
    rid[1].usUsage = 0x06;
    rid[1].dwFlags = 0;
    rid[1].hwndTarget = hwnd;
    
    if (!RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE))) {
        // Failed to register raw input devices
    }
}

void Input::Update() {
    s_PrevKeys = s_Keys;
    s_PrevMouseButtons = s_MouseButtons;
    s_MouseDeltaX = 0.0f;
    s_MouseDeltaY = 0.0f;
}

void Input::ProcessRawInput(LPARAM lParam) {
    UINT size = 0;
    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
    
    if (size == 0) return;
    
    auto rawData = std::make_unique<BYTE[]>(size);
    if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawData.get(), &size, sizeof(RAWINPUTHEADER)) != size) {
        return;
    }
    
    RAWINPUT* raw = (RAWINPUT*)rawData.get();
    
    if (raw->header.dwType == RIM_TYPEKEYBOARD) {
        USHORT vkey = raw->data.keyboard.VKey;
        bool isDown = !(raw->data.keyboard.Flags & RI_KEY_BREAK);
        
        if (vkey < 256) {
            s_Keys[vkey] = isDown;
        }
    }
    else if (raw->header.dwType == RIM_TYPEMOUSE) {
        if (raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE) {
            s_MouseDeltaX = static_cast<float>(raw->data.mouse.lLastX);
            s_MouseDeltaY = static_cast<float>(raw->data.mouse.lLastY);
        }
        
        USHORT buttonFlags = raw->data.mouse.usButtonFlags;
        if (buttonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) s_MouseButtons[0] = true;
        if (buttonFlags & RI_MOUSE_LEFT_BUTTON_UP) s_MouseButtons[0] = false;
        if (buttonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) s_MouseButtons[1] = true;
        if (buttonFlags & RI_MOUSE_RIGHT_BUTTON_UP) s_MouseButtons[1] = false;
        if (buttonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) s_MouseButtons[2] = true;
        if (buttonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) s_MouseButtons[2] = false;
    }
}

} // namespace Henky3D
