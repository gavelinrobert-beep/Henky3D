#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <array>

using Microsoft::WRL::ComPtr;

namespace Henky3D {

class GraphicsDevice;

// Ring buffer allocator for per-frame constant buffer data
class ConstantBufferAllocator {
public:
    static constexpr UINT FrameCount = 2;
    static constexpr UINT BufferSizePerFrame = 1024 * 1024; // 1MB per frame

    ConstantBufferAllocator(GraphicsDevice* device);
    ~ConstantBufferAllocator();

    // Allocate space for constant buffer data
    // Returns GPU virtual address for binding
    D3D12_GPU_VIRTUAL_ADDRESS Allocate(UINT sizeInBytes, void** cpuAddress);

    // Reset the allocator for a new frame
    void Reset();

private:
    GraphicsDevice* m_Device;
    ComPtr<ID3D12Resource> m_Buffer;
    UINT8* m_MappedData;
    UINT m_CurrentOffset;
    UINT m_FrameIndex;
};

} // namespace Henky3D
