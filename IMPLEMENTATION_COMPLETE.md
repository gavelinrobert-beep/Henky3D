# Implementation Summary: Materials, Shadows, and Frame Graph

## ✅ Delivered in this Slice
- OpenGL 4.6 core rendering with depth prepass + forward shading.
- Directional shadow map (2048²) with PCF sampling and adjustable bias/toggle.
- Material/texture registry with default fallback textures and PBR-friendly parameters.
- Per-frame/per-draw UBOs with fixed bindings for predictable layouts.
- Frame-graph scaffold for ordered pass execution.
- ImGui overlay exposing rendering controls and stats.

## Technical Highlights
- GL core profile only (no fixed-function); GLAD loader; GLFW window/context.
- GLSL 460 shaders with shared `Common.glsl` for constant blocks.
- ECS-driven rendering: `Transform`, `Renderable`, `Light`, `BoundingBox` components drive submission.
- Resize-safe swapchain handling via `GraphicsDevice::ResizeBuffers` + viewport updates.

## Outstanding Items (Tracked, Not Changed Here)
- Move renderer to DSA-only state management.
- Introduce RHI layer to isolate GL from higher-level renderer (future Vulkan slot-in).
- Add clustered/Forward+ lighting, GTAO/TAA/bloom/tonemap, and richer material permutations.
- Replace GLFW with a dedicated Win32 platform wrapper when the platform layer is expanded.
