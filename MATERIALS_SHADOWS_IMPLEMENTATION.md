# Materials, Textures, and Shadow Mapping Implementation

This document describes the implementation of materials, textures, shadow mapping, and frame-graph infrastructure for Henky3D.

## Overview

The implementation adds:
1. Material and texture asset system with WIC-based texture loading
2. Single-cascade shadow mapping with PCF filtering
3. Extended per-frame constants for lighting and shadows
4. Lightweight frame-graph scaffolding for future pass management
5. ImGui debug UI for shadows and rendering statistics

## 1. Material and Texture System

### Asset Registry (`AssetRegistry.h/cpp`)
Central registry for materials and textures:
- **Texture caching**: Textures are loaded once and cached by path
- **Default textures**: White (baseColor), flat normal (128,128,255), mid roughness/metalness
- **WIC texture loader**: Supports common image formats (PNG, JPG, BMP, etc.)
- **Format detection**: Automatically determines sRGB vs linear based on filename heuristics
  - BaseColor textures: sRGB (`DXGI_FORMAT_R8G8B8A8_UNORM_SRGB`)
  - Normal/RM textures: Linear (`DXGI_FORMAT_R8G8B8A8_UNORM`)

### Material Asset (`Material.h`)
```cpp
struct MaterialAsset {
    XMFLOAT4 BaseColorFactor;           // Base color multiplier
    TextureHandle BaseColorTexture;      // Optional baseColor texture
    TextureHandle NormalTexture;         // Optional normal map
    TextureHandle RoughnessMetalnessTexture; // Optional RM map (R=metalness, G=roughness)
    float RoughnessFactor;               // Default roughness (0-1)
    float MetalnessFactor;               // Default metalness (0-1)
    bool AlphaMask;                      // Use alpha testing
    float AlphaCutoff;                   // Alpha cutoff threshold
};
```

### Texture Packing Conventions
- **BaseColor**: sRGB color space (RGBA8_SRGB)
- **Normal**: Linear (RGBA8), tangent-space normals in [0,1] range (128,128,255 = up)
- **Roughness/Metalness**: Linear (RGBA8), R=metalness, G=roughness, B/A unused

## 2. Shadow Mapping

### ShadowMap Class (`ShadowMap.h/cpp`)
Single-cascade shadow map implementation:
- **Resolution**: Configurable (default 2048x2048)
- **Format**: `DXGI_FORMAT_R32_TYPELESS` for DSV, `DXGI_FORMAT_R32_FLOAT` for SRV
- **Light view-projection**: Computed via `ComputeLightViewProjection()`
  - Orthographic projection for directional lights
  - Tight fitting to scene bounds for optimal shadow quality

### Shadow Pass
Shadow pass renders depth-only from light's perspective:
1. **Transition** shadow map to `DEPTH_WRITE`
2. **Clear** depth to 1.0
3. **Render** all visible geometry with shadow PSO
4. **Transition** shadow map to `PIXEL_SHADER_RESOURCE`

### PCF Shadow Sampling
Forward pixel shader implements 3x3 PCF (Percentage Closer Filtering):
```hlsl
float SampleShadowMapPCF(float3 shadowPos) {
    // Perspective divide and transform to [0,1]
    shadowPos.xy /= shadowPos.w;
    shadowPos.xy = shadowPos.xy * 0.5 + 0.5;
    shadowPos.y = 1.0 - shadowPos.y;
    
    // Apply bias
    float depth = shadowPos.z - g_PerFrame.ShadowBias;
    
    // 3x3 PCF sampling
    float shadow = 0.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float2 offset = float2(x, y) * texelSize;
            shadow += g_ShadowMap.SampleCmpLevelZero(g_ShadowSampler, shadowPos.xy + offset, depth);
        }
    }
    return shadow / 9.0;
}
```

### Shadow Sampler
Comparison sampler with border color white (unshadowed outside bounds):
```cpp
Filter: COMPARISON_MIN_MAG_LINEAR_MIP_POINT
AddressMode: BORDER
ComparisonFunc: LESS_EQUAL
BorderColor: (1,1,1,1)
```

## 3. Extended Constant Buffers

### PerFrameConstants (256-byte aligned)
```cpp
struct PerFrameConstants {
    XMFLOAT4X4 ViewMatrix;
    XMFLOAT4X4 ProjectionMatrix;
    XMFLOAT4X4 ViewProjectionMatrix;
    XMFLOAT4X4 LightViewProjectionMatrix;  // For shadow mapping
    XMFLOAT4 CameraPosition;
    XMFLOAT4 LightDirection;               // Directional light
    XMFLOAT4 LightColor;                   // RGB = color, A = intensity
    XMFLOAT4 AmbientColor;                 // RGB = color, A = intensity
    float Time;
    float DeltaTime;
    float ShadowBias;                      // Configurable via ImGui
    float ShadowsEnabled;                  // 1.0 = on, 0.0 = off
};
```

## 4. Shader Updates

### Forward Vertex Shader (`Forward.vs.hlsl`)
- Outputs shadow position: `ShadowPos = mul(worldPos, g_PerFrame.LightViewProjectionMatrix)`
- Transforms to light clip space for shadow sampling

### Forward Pixel Shader (`Forward.ps.hlsl`)
Enhanced lighting model:
1. **Diffuse**: Lambert (`NdotL`) modulated by light color/intensity
2. **Specular**: Blinn-Phong with fixed shininess (32)
3. **Ambient**: Scene ambient color
4. **Shadow**: PCF-filtered shadow term applied to direct lighting

### Shadow Shaders (`Shadow.vs/ps.hlsl`)
Minimal depth-only shaders:
- Vertex shader: Transforms to light clip space
- Pixel shader: Empty (depth write only)

## 5. Frame-Graph Scaffolding

### FrameGraph Class (`FrameGraph.h/cpp`)
Lightweight pass management system:
```cpp
frameGraph.AddPass("Shadow", [](ID3D12GraphicsCommandList* cmd) {
    // Shadow rendering
});
frameGraph.AddPass("Forward", [](ID3D12GraphicsCommandList* cmd) {
    // Forward rendering
});
frameGraph.Execute(commandList);
```

Features:
- **Pass ordering**: Explicit pass sequencing
- **Enable/disable**: Runtime pass toggling via `SetPassEnabled()`
- **Statistics**: Track enabled pass count

Currently not integrated into main rendering loop - infrastructure ready for future use.

## 6. ImGui Debug UI

### Rendering Controls
- **Depth Prepass**: Toggle early-Z optimization
- **Shadows**: Enable/disable shadow rendering
- **Shadow Bias**: Adjust to reduce shadow acne/peter-panning (0.0 - 0.01)

### Statistics
- **Draw Calls**: Number of draw calls issued per frame
- **Culled**: Number of objects culled (currently 0 - culling not active)
- **Triangles**: Total triangles rendered

## 7. Renderer Integration

### Root Signature Updates
Main root signature now includes:
```
0: CBV (b0) - PerFrameConstants
1: CBV (b1) - PerDrawConstants
2: SRV Descriptor Table (t0) - Shadow map
3: Sampler Descriptor Table (s0) - Shadow sampler
```

Shadow root signature (simpler):
```
0: CBV (b0) - PerFrameConstants
1: CBV (b1) - PerDrawConstants
```

### Rendering Order
1. **BeginFrame()**: Reset constant buffer allocator
2. **SetPerFrameConstants()**: Upload lighting/camera data
3. **RenderShadowPass()**: Render depth from light (if enabled)
4. **RenderScene()**: Main forward rendering with shadows
5. **ImGui rendering**

### Resource Transitions
Proper barriers for shadow map:
- Shadow pass: `PIXEL_SHADER_RESOURCE` → `DEPTH_WRITE` → `PIXEL_SHADER_RESOURCE`
- Forward pass: Read shadow map as `PIXEL_SHADER_RESOURCE`

## 8. Configuration

### Shadow Map Resolution
Configurable in `Renderer` constructor:
```cpp
m_ShadowMap = std::make_unique<ShadowMap>(device, 2048); // 2048x2048
```

### Shadow Bias
Adjustable at runtime via ImGui slider:
- Range: 0.0 - 0.01
- Default: 0.005
- Controls depth bias to reduce shadow acne

### Scene Bounds (for light view-projection)
Currently hardcoded in `main.cpp`:
```cpp
XMFLOAT3 sceneBoundsMin = { -5.0f, -5.0f, -5.0f };
XMFLOAT3 sceneBoundsMax = { 5.0f, 5.0f, 5.0f };
```
Should be computed dynamically from scene geometry in production.

## Future Enhancements

### Immediate Next Steps
1. **Texture sampling in shaders**: Integrate material textures into forward pass
2. **Normal mapping**: Add TBN calculation and normal map sampling
3. **PBR shading**: Replace Blinn-Phong with GGX specular
4. **Alpha masking**: Add discard/clip based on alpha cutoff

### Longer Term
1. **Cascaded shadow maps**: Multi-cascade for large view distances
2. **Shadow filtering**: ESM, VSM, or PCSS for softer shadows
3. **Frame-graph integration**: Use FrameGraph for pass management
4. **Material inspector UI**: Browse/edit materials in ImGui
5. **Mesh loading**: Load geometry from GLTF/OBJ files
6. **KTX2/BCn support**: Compressed texture formats

## Technical Notes

### Descriptor Heap Management
- **RTV Heap**: 2 descriptors (double-buffered back buffers)
- **DSV Heap**: 10 descriptors (main depth + shadow maps)
- **SRV Heap**: 1000 descriptors (GPU-visible, ring-buffered per frame)
- **Sampler Heap**: 10 descriptors (shadow sampler + future texture samplers)

### Memory Layout
All constant buffers are 256-byte aligned per D3D12 requirements.

### Shader Model
All shaders compiled to **Shader Model 6.6** for latest feature support.

### Build Requirements
- Windows 10/11 (DirectX 12)
- Visual Studio 2019+ with C++20
- Windows SDK 10.0.19041.0+
- DXC compiler (dxcompiler.dll)

## Testing

Run the application:
```batch
build\bin\Release\Henky3D.exe
```

Expected behavior:
- 3 rotating cubes with colored shading
- Directional light from above-right
- Shadows cast on ground plane (when enabled)
- ImGui overlay with controls and stats

Verify:
1. Toggle shadows on/off - should see shadow disappear
2. Adjust shadow bias - should affect shadow appearance
3. Stats should show draw calls and triangle count
4. Depth prepass toggle should work as before
