# Rendering and Scene Systems Implementation

This document describes the foundational rendering and scene systems implemented for Henky3D.

## Overview

The implementation adds:
1. Per-frame and per-draw constant buffer system with upload-ring allocator
2. Depth prepass and forward rendering passes with file-based HLSL shaders
3. Indexed cube geometry replacing the hardcoded triangle
4. Transform hierarchy with parent/child support and dirty propagation
5. Frustum culling system for visibility determination

## 1. Constant Buffer System

### ConstantBufferAllocator
- **Location**: `src/engine/graphics/ConstantBufferAllocator.h/cpp`
- **Purpose**: Ring buffer allocator for per-frame constant data
- **Features**:
  - 1MB per frame buffer (2MB total for double-buffering)
  - 256-byte alignment for D3D12 requirements
  - Auto-reset per frame via `Reset()` called from `BeginFrame()`
  - Returns GPU virtual address for binding

### Constant Buffer Structures
- **Location**: `src/engine/graphics/ConstantBuffers.h`

#### PerFrameConstants (256-byte aligned)
- ViewMatrix, ProjectionMatrix, ViewProjectionMatrix
- CameraPosition (for specular calculations)
- Time, DeltaTime

#### PerDrawConstants (256-byte aligned)
- WorldMatrix (for transforming vertices)
- MaterialIndex (placeholder for future material system)

### Root Signature
- **CBV at register b0**: Per-frame constants
- **CBV at register b1**: Per-draw constants

## 2. Shader System

### HLSL Shaders
Located in `shaders/` directory:

#### Depth Prepass
- `DepthPrepass.vs.hlsl`: Transforms positions to clip space
- `DepthPrepass.ps.hlsl`: Empty (depth-only pass)
- **Purpose**: Write depth values for early Z-rejection in forward pass

#### Forward Pass
- `Forward.vs.hlsl`: Full vertex transformation with normals
- `Forward.ps.hlsl`: Blinn-Phong lighting with simple directional light
- **Features**: 
  - Diffuse shading based on normal and light direction
  - Specular highlights
  - Ambient term

### Shader Loading
- File-based loading from `shaders/` relative to executable
- DXC compilation to SM 6.6
- Include handler support for future shader includes

### Pipeline States

#### Depth Prepass PSO
- Position-only input layout
- No color writes (`RenderTargetWriteMask = 0`)
- Depth write enabled with LESS comparison
- No render targets (depth-only)

#### Forward PSO (with prepass)
- Full vertex layout (Position, Normal, Color)
- Depth write **disabled** (`DEPTH_WRITE_MASK_ZERO`)
- Depth comparison function: **EQUAL**
- Benefits from depth prepass for pixel rejection

#### Forward No-Prepass PSO
- Same vertex layout as forward
- Depth write **enabled** (`DEPTH_WRITE_MASK_ALL`)
- Depth comparison function: **LESS**
- Used when depth prepass is disabled

### Runtime Toggle
- ImGui checkbox to enable/disable depth prepass
- Automatically selects correct forward PSO based on toggle state

## 3. Geometry System

### Cube Mesh
Replaces the hardcoded triangle with an indexed cube:
- **24 vertices** (4 per face for proper normals)
- **36 indices** (6 faces × 2 triangles × 3 indices)
- Each face has unique normal and color tint
- Centered at origin with extents [-0.5, 0.5]

### Vertex Structure
```cpp
struct Vertex {
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT4 Color;
};
```

### Rendering
- Index buffer support added to Renderer
- DrawIndexedInstanced used for all geometry
- Per-draw constants allocated for each draw call

## 4. Transform Hierarchy System

### Enhanced Transform Component
- **Location**: `src/engine/ecs/Components.h`

#### New Features
- `Parent` entity reference for hierarchy
- Cached `WorldMatrix` (XMFLOAT4X4)
- `Dirty` flag for change tracking
- `GetLocalMatrix()`: Computes TRS matrix
- `GetMatrix()`: Returns cached world matrix
- `MarkDirty()`: Invalidates cache and triggers update

### TransformSystem
- **Location**: `src/engine/ecs/TransformSystem.h/cpp`

#### Functions
- `UpdateTransforms(ECSWorld*)`: Entry point, updates all root transforms
- `UpdateTransformRecursive(...)`: Recursively updates children

#### Algorithm
1. Iterate all transforms to find roots (Parent == entt::null)
2. For each root, compute world matrix = local × parent (identity for roots)
3. Recursively update all children with parent's world matrix
4. Clear dirty flag after update
5. Children are found by iterating all transforms and checking Parent field

## 5. Frustum Culling System

### Frustum Component
- **Location**: `src/engine/ecs/Components.h`

#### Features
- 6 planes (Left, Right, Bottom, Top, Near, Far)
- `ExtractFromMatrix()`: Extracts planes from view-projection matrix
- `TestBox()`: Tests AABB against frustum using separating axis theorem

### BoundingBox Component
- Min/Max extents in local space
- Default: cube with extents [-0.5, 0.5]
- `GetCenter()`, `GetExtents()` helper methods

### CullingSystem
- **Location**: `src/engine/ecs/CullingSystem.h/cpp`

#### Functions
- `CullEntities(ECSWorld*, Frustum)`: Returns list of visible entities

#### Algorithm
1. Query all entities with Transform, Renderable, and BoundingBox
2. Skip if Renderable.Visible is false
3. Transform bounding box to world space:
   - Transform center with world matrix
   - Conservative scale extents by max scale component
4. Test world-space box against frustum
5. Return list of visible entity IDs

## 6. Integration

### Main Application Updates
- **Location**: `src/main.cpp`

#### Changes
- Import new systems (TransformSystem, CullingSystem, ConstantBuffers)
- Initialize 3 cubes in scene (center, left, right)
- Add BoundingBox to all renderable entities
- Update camera aspect ratio on window resize
- Call TransformSystem::UpdateTransforms() every frame
- Setup PerFrameConstants with camera matrices
- Call Renderer::RenderScene() with depth prepass toggle
- Add rotation animation to first cube
- Add depth prepass toggle to ImGui UI
- Track total time for shader time uniform

### Renderer Updates
- **Location**: `src/engine/graphics/Renderer.h/cpp`

#### New Methods
- `BeginFrame()`: Resets constant buffer allocator
- `SetPerFrameConstants()`: Uploads per-frame data
- `DrawCube()`: Draws indexed cube with per-draw constants
- `RenderScene()`: Renders all visible entities with optional depth prepass

#### Rendering Flow
1. BeginFrame() resets CB allocator
2. SetPerFrameConstants() uploads camera data
3. If depth prepass enabled:
   - Set depth prepass PSO
   - Render all visible entities (depth-only)
4. Set forward PSO (with or without prepass)
5. Render all visible entities (color + shading)

## 7. Build System

### CMake Updates
- **Location**: `src/engine/CMakeLists.txt`

#### New Sources
- ConstantBufferAllocator.cpp/h
- ConstantBuffers.h
- TransformSystem.cpp/h
- CullingSystem.cpp/h

### Shader Deployment
Shaders are loaded from `shaders/` directory relative to executable. The path resolution:
1. Get executable path via GetModuleFileNameW()
2. Navigate up to project root
3. Append `shaders/` + filename

## Key Design Decisions

### Constant Buffer Allocation
- Ring buffer approach avoids per-frame resource creation
- Aligned to 256 bytes for D3D12 requirements
- GPU virtual addresses allow efficient binding without descriptor tables

### Dual Forward PSOs
- Separate PSOs for depth prepass enabled/disabled avoids runtime checks
- EQUAL comparison in prepass mode ensures only visible fragments are shaded
- Toggle doesn't require pipeline recreation

### Transform Hierarchy
- Cached world matrices avoid redundant calculations
- Dirty propagation ensures updates only when needed
- O(n) update with n = total transforms (not ideal for deep hierarchies, but simple)

### Frustum Culling
- Conservative AABB transformation (max scale) ensures no false negatives
- Currently infrastructure only (not actively filtering in RenderScene)
- Can be enabled by replacing `view` iteration with culled entity list

## Future Enhancements

1. **Active Frustum Culling**: Use CullingSystem results to filter rendering
2. **Material System**: Utilize MaterialIndex in PerDrawConstants
3. **Shader Includes**: Add common shader code (lighting functions, etc.)
4. **PSO Caching**: Cache compiled shaders to avoid reload
5. **Async Shader Compilation**: Compile shaders on background thread
6. **Transform Hierarchy Optimization**: Spatial hashing for child lookup
7. **Multiple Lights**: Pass light array to shaders
8. **Shadow Mapping**: Add shadow pass after depth prepass

## Testing Notes

The implementation is complete but requires a Windows environment to build and test:
- DX12 API (Windows 10/11 only)
- DXC shader compiler (dxcompiler.dll)
- Visual Studio 2019+ with C++20 support
- Windows SDK 10.0.19041.0+

When built, the application should:
- Display 3 colored cubes (red/center, green/right, blue/left)
- Center cube rotates continuously
- Depth prepass can be toggled in ImGui UI
- Camera controls work as before (WASD + mouse)
- Window resize works correctly with updated viewport/scissor
