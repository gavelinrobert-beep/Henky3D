# Rendering and Scene Systems (OpenGL 4.6 Core)

This document captures the current rendering/scene slice implemented for the AURORA Engine baseline.

## Overview
- Depth prepass (optional) + forward shading (GLSL 460 core).
- Directional shadow map (2048²) with 3×3 PCF and configurable bias.
- Per-frame/per-draw UBOs (std140, bindings 0/1).
- VAO/VBO/IBO cube geometry; GL core profile only.
- Lightweight frame-graph scaffold for ordered pass execution.

## Constant Data
- **PerFrameConstants**: view, projection, view-projection, light view-projection, camera position, light direction/color, ambient color, time/delta, shadow bias, shadows enabled flag.
- **PerDrawConstants**: world matrix, material index placeholder.
Uniform buffers are updated via `glBufferSubData` and bound once to bindings 0/1.

## Passes
1. **Shadow Pass** (optional): renders all visible renderables into a depth-only FBO owned by `ShadowMap`; PCF sampling in the forward pass.
2. **Depth Prepass** (optional): writes depth only to prime early-Z; color writes masked off.
3. **Forward Pass**: Blinn-Phong lighting with ambient + directional diffuse/specular; optional shadow sampling; resets depth func if prepass was enabled.
4. **ImGui**: GLFW/OpenGL3 backend render after scene.

## Geometry
- Indexed cube (24 verts / 36 indices) with position/normal/color attributes in a single VAO/VBO/IBO.
- Draws with `glDrawElements` in both shadow and forward passes.

## Frame Graph
- `FrameGraph` collects named passes with enable flags; executes in order each frame.
- Currently used to keep pass ordering explicit; resource lifetime/barriers are simple because GL handles hazards implicitly, with manual state setup.

## Integration Points
- Renderer consumes ECS data (`Transform`, `Renderable`, `Light`, `BoundingBox`).
- Shadow map and per-frame constants are set up in `main.cpp` before issuing passes.
- Resizing propagates through the window callback to the device and viewport.

## Known Gaps vs AURORA Target
- No DSA-only path yet (bind-to-edit is used throughout).
- No RHI abstraction; renderer speaks OpenGL directly.
- No clustered/Forward+, GTAO/TAA/bloom/tonemap, or material permutation controls yet.
- Platform layer relies on GLFW instead of a bespoke Win32 wrapper.

These gaps are intentionally left for future slices; current code is stable for the implemented feature set.
