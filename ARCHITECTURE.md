# Henky3D / AURORA Architecture (Current Slice)

## System Overview
```
Application (main.cpp)
 ├─ Window / Timer (core)
 ├─ Input (keyboard/mouse)
 ├─ ECS World (EnTT)
 │   ├─ Components: Transform, Camera, Renderable, Light, BoundingBox
 │   ├─ Systems: Transform hierarchy update, culling hooks
 └─ Renderer (graphics)
     ├─ GraphicsDevice (GLFW + GLAD, OpenGL 4.6 core)
     ├─ FrameGraph (ordered passes)
     ├─ ShadowMap (directional depth map)
     ├─ AssetRegistry (materials/textures)
     └─ ImGui overlay
```

## Render Loop (OpenGL Core Profile)
1. **BeginFrame**
   - Clear color/depth, reset stats, bind viewport.
2. **Shadow Pass (optional)**
   - Bind shadow FBO, render all renderables depth-only with per-draw UBO.
3. **Depth Prepass (optional)**
   - Mask color writes, render all renderables to populate depth.
4. **Forward Pass**
   - Bind forward program; set per-frame UBO (view/proj/light/shadow); bind shadow map if enabled; draw all visible renderables.
5. **ImGui**
   - Build overlay (stats + toggles) and render via ImGui OpenGL3 backend.
6. **EndFrame**
   - Swap buffers via GLFW; optional `glFinish` on wait.

## Memory & Data
- **Buffers**: Per-frame/per-draw UBOs (std140, bindings 0/1). VAO/VBO/IBO for cube geometry.
- **Textures**: 2D GL textures managed by `AssetRegistry`; default fallback textures; shadow depth texture.
- **State**: Core profile only, depth test + face culling enabled; vsync via GLFW.

## Compliance Notes vs AURORA Spec
- ✅ C++20, CMake, OpenGL 4.5+ core, GLAD/GLFW/ImGui/EnTT; depth prepass + shadow map; ECS-driven renderer; frame-graph scaffold.
- ⚠️ DSA-only state management not yet in place (uses bind-to-edit).
- ⚠️ Platform layer uses GLFW instead of a bespoke Win32 wrapper.
- ⚠️ RHI abstraction and Vulkan-ready split are future work.
- ⏳ Forward+/clustered lighting, GTAO/TAA/bloom/tonemap, and editor separation are planned but not yet implemented.
