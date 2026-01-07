# Henky3D Engine Architecture

## System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                      Application (main.cpp)                      │
│  - Main loop                                                     │
│  - Frame timing                                                  │
│  - Event handling                                                │
└────────────┬────────────┬────────────┬────────────┬─────────────┘
             │            │            │            │
             ▼            ▼            ▼            ▼
      ┌───────────┐ ┌──────────┐ ┌─────────┐ ┌──────────┐
      │  Window   │ │  Input   │ │   ECS   │ │ Graphics │
      │  System   │ │  System  │ │  World  │ │  Device  │
      └───────────┘ └──────────┘ └─────────┘ └──────────┘
             │            │            │            │
             │            │            │            ▼
             │            │            │     ┌──────────────┐
             │            │            │     │  Renderer    │
             │            │            │     │  (Triangle)  │
             │            │            │     └──────────────┘
             │            │            │            │
             │            │            ▼            ▼
             │            │     ┌────────────────────────┐
             │            │     │  Components            │
             │            │     │  - Transform           │
             │            │     │  - Camera              │
             │            │     │  - Renderable          │
             │            │     │  - Light               │
             │            │     └────────────────────────┘
             │            │
             │            └────────────────────────────────┐
             │                                             │
             ▼                                             ▼
      ┌────────────────┐                          ┌──────────────┐
      │  Win32 Window  │                          │  Raw Input   │
      │  - WndProc     │                          │  - Keyboard  │
      │  - Messages    │                          │  - Mouse     │
      └────────────────┘                          └──────────────┘
             │
             ▼
      ┌────────────────────────────────────────────────────────────┐
      │                    ImGui Overlay                            │
      │  - FPS Display                                              │
      │  - Camera Controls                                          │
      │  - Debug Info                                               │
      └────────────────────────────────────────────────────────────┘
```

## DirectX 12 Pipeline

```
┌─────────────────────────────────────────────────────────────────┐
│                      GraphicsDevice                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ID3D12Device                                                    │
│     │                                                            │
│     ├─── Command Queue (Graphics)                               │
│     │       └── Execute command lists                            │
│     │                                                            │
│     ├─── Command Queue (Compute)                                │
│     │       └── Async compute workloads                          │
│     │                                                            │
│     ├─── Command Queue (Copy)                                   │
│     │       └── Upload operations                                │
│     │                                                            │
│     ├─── Swapchain (Flip Model)                                 │
│     │       ├── Render Target 0 (Back Buffer)                   │
│     │       └── Render Target 1 (Back Buffer)                   │
│     │                                                            │
│     ├─── Descriptor Heaps                                       │
│     │       ├── RTV Heap (Render Target Views)                  │
│     │       ├── DSV Heap (Depth Stencil View)                   │
│     │       └── SRV Heap (GPU-visible ring for transient SRVs)  │
│     │                                                            │
│     ├─── Depth Stencil Buffer                                   │
│     │       └── D32_FLOAT format                                │
│     │                                                            │
│     └─── Fence + Timeline                                       │
│             └── Frame synchronization                            │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## Render Loop

```
1. BeginFrame()
   ├── Reset command allocator
   └── Reset command list

2. Transition RT: PRESENT -> RENDER_TARGET

3. Clear render target (sRGB)
   └── Clear depth stencil

4. Set render targets (RTV + DSV)

5. Set viewport and scissor rect

6. Draw triangle
   ├── Set root signature
   ├── Set pipeline state
   ├── Set vertex buffer
   └── Draw call

7. Render ImGui overlay
   ├── ImGui frame setup
   ├── Build UI
   └── Render draw data

8. Transition RT: RENDER_TARGET -> PRESENT

9. EndFrame()
   ├── Close command list
   ├── Execute on queue
   ├── Present (VSync)
   └── Wait for previous frame
```

## ECS Update Flow

```
Application::Update(deltaTime)
    │
    ├─── Input::Update()
    │       └── Update key/mouse states
    │
    ├─── UpdateCamera(deltaTime)
    │       ├── Process WASD movement
    │       ├── Process mouse look
    │       └── Update camera matrices
    │
    └─── ECSWorld::Update(deltaTime)
            └── [System update stub for future expansion]
                ├── Physics system
                ├── Animation system
                ├── Transform hierarchy
                └── Render preparation
```

## Memory Management

- **Command Allocators**: Per-frame allocators (double-buffered)
- **Upload Buffers**: Persistently mapped for efficiency
- **Render Targets**: Managed by swapchain
- **Depth Buffer**: Single buffer, recreated on resize
- **ImGui Resources**: Static descriptor heap allocation
- **Fence Values**: Per-frame tracking for GPU synchronization
