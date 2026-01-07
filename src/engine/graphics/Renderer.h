#pragma once
#include "GraphicsDevice.h"
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace Henky3D {

class Renderer {
public:
    Renderer(GraphicsDevice* device);
    ~Renderer();

    void RenderTriangle();

private:
    void CreatePipelineState();
    void CreateVertexBuffer();

    GraphicsDevice* m_Device;
    ComPtr<ID3D12RootSignature> m_RootSignature;
    ComPtr<ID3D12PipelineState> m_PipelineState;
    ComPtr<ID3D12Resource> m_VertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
};

} // namespace Henky3D
