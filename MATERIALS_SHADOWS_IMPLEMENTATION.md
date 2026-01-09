# Materials, Textures, and Shadows (OpenGL 4.6)

This slice documents the current material/texture registry, shadow mapping, and related rendering controls.

## Asset Registry
- **Textures**: OpenGL 2D textures managed by handles; loaded via stb (KTX2/GLTF loaders planned). Textures are cached by path; defaults include white, flat normal, and mid roughness/metalness.
- **Materials**: `MaterialAsset` holds base color factor/texture, normal map, roughness/metalness factors + packed texture, alpha mask settings, and helper flags. Materials are stored in a contiguous array for GPU-friendly indexing.

## Shadow Mapping
- **ShadowMap**: 2048×2048 depth texture + FBO; computes light view-projection for a directional light.
- **Sampling**: Fragment shader performs 3×3 PCF (percentage closer filtering) with adjustable bias (`ShadowBias`) and toggle (`ShadowsEnabled`).
- **Pass Flow**:
  1. `ShadowMap::BeginShadowPass()` binds FBO, clears depth, sets viewport.
  2. Renderer draws all visible renderables with depth-only shader.
  3. `ShadowMap::EndShadowPass()` restores default framebuffer.

## Rendering Controls & Stats
- **ImGui**: Toggles for shadows and depth prepass, bias slider, and stats (draw calls, culled count, triangle count).
- **Constants**: `PerFrameConstants` carries light view-projection, light parameters, ambient, timing, bias, and shadow toggle; `PerDrawConstants` carries world matrix + material index.

## Known Gaps / Future Work
- Texture streaming, KTX2/BCn ingestion, and bindless/buffered descriptor emulation are not implemented yet.
- Material permutations and shader specialization (PBR, clustered/Forward+) are future iterations.
- Cascaded shadow maps and shadow atlas management are not present; current implementation is a single cascade suitable for the sample scene.
