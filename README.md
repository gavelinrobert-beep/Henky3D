# Henky3D
Minimal PC Graphics Engine with DirectX 12

## Features
- Win32 window system with message loop
- High-resolution timer utilities
- Raw mouse and keyboard input handling
- DirectX 12 rendering pipeline
  - Device and command queue setup
  - Flip-model swapchain with sRGB output
  - RTV/DSV descriptor heaps
  - Upload buffer helpers
  - Depth buffer support
  - Fence-based GPU synchronization
- Test triangle rendering
- EnTT Entity Component System
  - Transform, Camera, Renderable, Light components
  - System update order stub
- ImGui overlay
  - FPS counter and frame time display
  - Camera fly controls (WASD movement, mouse look)

## Requirements
- Windows 10/11
- CMake 3.20+
- Visual Studio 2019+ with C++20 support
- Windows SDK with DirectX 12

## Building
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Running
```bash
build/bin/Release/Henky3D.exe
```

## Controls
- Enable camera control via ImGui checkbox
- WASD: Move camera
- Q/E: Move down/up
- Mouse: Look around
- Adjust camera speed via ImGui sliders
