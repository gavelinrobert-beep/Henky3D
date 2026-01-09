#include "ConstantBufferAllocator.h"
#include "GraphicsDevice.h"
#include <stdexcept>

namespace Henky3D {

ConstantBufferAllocator::ConstantBufferAllocator(GraphicsDevice* device)
    : m_Device(device), m_MappedData(nullptr), m_CurrentOffset(0), m_FrameIndex(0) {
    
    const UINT64 totalSize = BufferSizePerFrame * FrameCount;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = totalSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if (FAILED(m_Device->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_Buffer)))) {
        throw std::runtime_error("Failed to create constant buffer allocator");
    }

    D3D12_RANGE readRange = { 0, 0 };
    if (FAILED(m_Buffer->Map(0, &readRange, reinterpret_cast<void**>(&m_MappedData)))) {
        throw std::runtime_error("Failed to map constant buffer allocator");
    }
}

ConstantBufferAllocator::~ConstantBufferAllocator() {
    if (m_Buffer && m_MappedData) {
        m_Buffer->Unmap(0, nullptr);
    }
}

D3D12_GPU_VIRTUAL_ADDRESS ConstantBufferAllocator::Allocate(UINT sizeInBytes, void** cpuAddress) {
    // Align to 256 bytes (D3D12 constant buffer requirement)
    const UINT alignment = 256;
    const UINT alignedSize = (sizeInBytes + alignment - 1) & ~(alignment - 1);

    // Check if we have enough space in current frame's buffer
    const UINT available = BufferSizePerFrame - m_CurrentOffset;
    if (alignedSize > available) {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg), 
            "Constant buffer allocator exhausted: requested %u bytes (aligned: %u), available %u bytes in frame %u",
            sizeInBytes, alignedSize, available, m_FrameIndex);
        throw std::runtime_error(errorMsg);
    }

    const UINT frameOffset = m_FrameIndex * BufferSizePerFrame;
    const UINT offset = frameOffset + m_CurrentOffset;

    *cpuAddress = m_MappedData + offset;
    m_CurrentOffset += alignedSize;

    return m_Buffer->GetGPUVirtualAddress() + offset;
}

void ConstantBufferAllocator::Reset() {
    m_FrameIndex = m_Device->GetCurrentFrameIndex();
    m_CurrentOffset = 0;
}

} // namespace Henky3D
