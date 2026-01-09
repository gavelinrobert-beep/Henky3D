# AURORA Engine (OpenGL 4.6 Core)

PC-first engine foundation aligned with the AURORA constraints: C++20, Windows-first, OpenGL 4.5+ core profile, and a minimal but scalable module layout under `src/engine`.

## Current Capabilities
- **Platform & Windowing**: GLFW-backed window (Win32 on Windows) with resize handling, vsync, and high-resolution timer utilities.
- **Rendering**: Modern OpenGL 4.6 core via GLAD, depth prepass + forward shading, single directional light with shadow map (2048x2048), per-frame/per-draw UBOs, VAO/VBO/IBO geometry, and basic frame-graph scaffold.
- **ECS**: EnTT-based world with `Transform`, `Camera`, `Renderable`, `Light`, and `BoundingBox` components plus transform hierarchy updates and simple frustum culling hooks.
- **ImGui Overlay**: Stats (FPS, draw calls, triangles, culled), depth-prepass and shadow toggles/bias, camera fly-control toggles and tuning.
- **Sample Scene**: Three cubes with colored faces, directional light, optional shadows, and optional camera fly controls.

## Requirements
- **OS**: Windows 10/11 64-bit (Linux builds work for development; target remains PC/Win32).
- **GPU**: OpenGL 4.5+ driver (4.6 core context requested).
- **Build Tools**: CMake 3.20+ and a C++20 compiler (MSVC 2019+, recent Clang/GCC).
- **Dependencies**: Fetched automatically (GLFW 3.4, GLM 1.0.1, ImGui v1.91.5, EnTT header-only, GLAD, OpenGL).

## Building

### Option 1: Using the build script (Windows)
```batch
build.bat
```

### Option 2: Manual CMake
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

The executable will be located at `build/bin/Release/Henky3D.exe` (or `build/bin/Henky3D` on non-Windows dev machines).

## Running
```bash
build/bin/Release/Henky3D.exe   # Windows
# or
./build/bin/Henky3D             # Linux/macOS development
```

## Controls
1. Toggle **Enable Camera Control** in ImGui.
2. **WASD**: Move camera; **Q/E**: Down/Up.
3. **Mouse**: Look (when camera control enabled).
4. Adjust **Move Speed** and **Look Speed** via ImGui.

## Project Structure
```
Henky3D/
├── src/
│   ├── main.cpp
│   └── engine/
│       ├── core/       # Window, Timer
│       ├── graphics/   # GraphicsDevice, Renderer, FrameGraph, ShadowMap, materials
│       ├── input/      # Input handling
│       └── ecs/        # Components, ECSWorld, systems
├── shaders/            # GLSL 460 core shaders (forward, depth prepass, shadow)
├── external/           # GLAD, GLFW, EnTT, ImGui (auto-fetched if missing)
└── CMakeLists.txt
```

## AURORA Alignment & Known Gaps
- ✅ C++20, OpenGL 4.5+ core (4.6 requested), GLAD loader, ImGui, EnTT, GLTF/KTX loaders planned.
- ✅ Depth prepass, directional shadow map, UBO-based constants, frame-graph scaffold, ECS-driven renderer.
- ⚠️ Platform layer currently uses GLFW rather than a pure Win32 wrapper.
- ⚠️ Renderer uses traditional binding workflow instead of full DSA-only state management.
- ⚠️ No Vulkan/RHI abstraction layer yet; renderer speaks OpenGL directly.
- ⚠️ Forward+ clustering, GTAO/SSA0, PBR material permutations, and editor isolation are future work.

These gaps are documented for follow-up; the current code remains stable and functional within the existing scope.
