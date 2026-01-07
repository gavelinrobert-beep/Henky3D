# Henky3D
Minimal PC Graphics Engine with DirectX 12

## Features
- **Win32 Window System**: Native window with message loop and event handling
- **High-Resolution Timer**: Precise frame timing and FPS counter utilities
- **Raw Input**: Raw mouse and keyboard input handling for responsive controls
- **DirectX 12 Rendering Pipeline**:
  - Device and command queue setup (graphics + compute + copy queues)
  - Flip-model swapchain with sRGB color space output
  - RTV/DSV descriptor heaps with allocation helpers
  - Upload buffer helper utilities
  - Depth buffer with D32_FLOAT format
  - Fence-based GPU synchronization and timeline management
  - Per-frame GPU-visible SRV heap ring for transient descriptors
- **Test Triangle Rendering**: Simple colored triangle with sRGB output
- **EnTT Entity Component System**:
  - Transform component with position, rotation, scale
  - Camera component with view/projection matrices and fly controls
  - Renderable component for visible geometry
  - Light component supporting directional, point, and spot lights
  - System update order stub for extensible game logic
- **ImGui Overlay**:
  - Real-time FPS counter and frame time display
  - Camera fly controls (WASD movement, mouse look)
  - Adjustable camera parameters (speed, sensitivity)
  - Dockable debug windows

## Requirements
- **OS**: Windows 10/11 (64-bit)
- **Build Tools**: CMake 3.20 or higher
- **Compiler**: Visual Studio 2019+ with C++20 support
- **SDK**: Windows SDK 10.0.19041.0 or newer (for DirectX 12)

## Building

### Option 1: Using the build script (Recommended)
```batch
build.bat
```

### Option 2: Manual CMake
```batch
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

The executable will be located at: `build/bin/Release/Henky3D.exe`

## Running
```batch
build\bin\Release\Henky3D.exe
```

Or double-click the executable in Windows Explorer.

## Controls
1. Check the **"Enable Camera Control"** checkbox in the ImGui overlay
2. **WASD**: Move camera forward/left/back/right
3. **Q/E**: Move camera down/up
4. **Mouse**: Look around (when camera control enabled)
5. Adjust **Move Speed** and **Look Speed** via ImGui sliders

## Project Structure
```
Henky3D/
├── src/
│   ├── main.cpp                    # Application entry point
│   └── engine/
│       ├── core/
│       │   ├── Window.h/cpp        # Win32 window management
│       │   └── Timer.h             # High-resolution timer
│       ├── graphics/
│       │   ├── GraphicsDevice.h/cpp # DX12 device and swapchain
│       │   └── Renderer.h/cpp      # Triangle rendering
│       ├── input/
│       │   └── Input.h/cpp         # Raw input handling
│       └── ecs/
│           ├── Components.h        # ECS component definitions
│           └── ECSWorld.h          # EnTT registry wrapper
├── external/
│   ├── entt/                       # EnTT v3.13.2
│   └── imgui/                      # ImGui v1.91.5
└── CMakeLists.txt
```

## Technical Details

### DirectX 12 Setup
- Uses flip model swapchain for better performance and tearing support
- Render targets use sRGB format (DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) for correct gamma
- Depth buffer uses 32-bit float format for high precision
- Double-buffered with frame synchronization via fences

### ECS Architecture
- Built on EnTT for high-performance entity management
- Components are plain data structures
- System update order is stubbed for future expansion
- Camera component includes fly controls for debugging

### ImGui Integration
- Integrated with DirectX 12 backend
- Persistent window layout via imgui.ini
- SRV descriptor heap allocated for ImGui textures
- Proper frame synchronization with rendering

## Development Notes
- All code uses C++20 features
- DirectX 12 Debug Layer enabled in Debug builds
- Resource barriers handle state transitions with tracked swapchain states
- Upload buffers remain mapped for efficient updates
- Inline shaders compiled with DXC targeting Shader Model 6

## Future Enhancements
This is a minimal scaffold. Potential additions:
- Asset loading system (models, textures, shaders)
- Material system with PBR shading
- Shadow mapping and lighting system
- Scene graph with transform hierarchy
- Physics integration
- Audio system
- Multi-threaded rendering
- Compute shader support

## License
See LICENSE file for details.
