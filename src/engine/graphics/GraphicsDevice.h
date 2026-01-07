#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <array>

using Microsoft::WRL::ComPtr;

namespace Henky3D {

class Window;

struct UploadBuffer {
    ComPtr<ID3D12Resource> Resource;
    void* MappedData = nullptr;
    UINT64 Size = 0;
};

struct DescriptorHandle {
    D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle{};
    D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle{};
};

class GraphicsDevice {
public:
    static constexpr UINT FrameCount = 2;
    static constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    static constexpr DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D32_FLOAT;

    GraphicsDevice(Window* window);
    ~GraphicsDevice();

    void BeginFrame();
    void EndFrame();
    void WaitForGPU();
    void ResizeBuffers(uint32_t width, uint32_t height);

    ID3D12Device* GetDevice() const { return m_Device.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return m_CommandList.Get(); }
    ID3D12CommandQueue* GetCommandQueue() const { return m_CommandQueue.Get(); }
    ID3D12CommandQueue* GetCopyQueue() const { return m_CopyQueue.Get(); }
    ID3D12CommandQueue* GetComputeQueue() const { return m_ComputeQueue.Get(); }
    IDXGISwapChain3* GetSwapChain() const { return m_SwapChain.Get(); }
    
    UINT GetCurrentFrameIndex() const { return m_FrameIndex; }
    UINT GetBackBufferIndex() const { return m_SwapChain->GetCurrentBackBufferIndex(); }
    
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const;
    ID3D12DescriptorHeap* GetSrvHeap() const { return m_SRVHeap.Get(); }
    ID3D12DescriptorHeap* GetDsvHeap() const { return m_DSVHeap.Get(); }
    
    UploadBuffer CreateUploadBuffer(UINT64 size);
    UINT AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type);
    DescriptorHandle AllocateSrvDescriptor(UINT count = 1);
    D3D12_CPU_DESCRIPTOR_HANDLE AllocateDsvDescriptor();

    void TransitionBackBuffer(ID3D12GraphicsCommandList* commandList, UINT bufferIndex, D3D12_RESOURCE_STATES newState);
    void TransitionBackBufferToRenderTarget(ID3D12GraphicsCommandList* commandList);
    void TransitionBackBufferToPresent(ID3D12GraphicsCommandList* commandList);

private:
    void CreateDeviceAndQueues();
    void CreateSwapChain(Window* window);
    void CreateDescriptorHeaps();
    void CreateRenderTargets();
    void CreateDepthStencil(uint32_t width, uint32_t height);
    void CreateCommandObjects();
    void CreateFences();

    ComPtr<IDXGIFactory6> m_Factory;
    ComPtr<ID3D12Device> m_Device;
    ComPtr<ID3D12CommandQueue> m_CommandQueue;
    ComPtr<ID3D12CommandQueue> m_ComputeQueue;
    ComPtr<ID3D12CommandQueue> m_CopyQueue;
    ComPtr<IDXGISwapChain3> m_SwapChain;
    
    ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
    ComPtr<ID3D12DescriptorHeap> m_DSVHeap;
    ComPtr<ID3D12DescriptorHeap> m_SRVHeap;
    
    ComPtr<ID3D12Resource> m_RenderTargets[FrameCount];
    ComPtr<ID3D12Resource> m_DepthStencil;
    
    ComPtr<ID3D12CommandAllocator> m_CommandAllocators[FrameCount];
    ComPtr<ID3D12GraphicsCommandList> m_CommandList;
    
    ComPtr<ID3D12Fence> m_Fence;
    UINT64 m_FenceValues[FrameCount];
    HANDLE m_FenceEvent;
    
    UINT m_RTVDescriptorSize;
    UINT m_DSVDescriptorSize;
    UINT m_SRVDescriptorSize;
    UINT m_FrameIndex;
    
    uint32_t m_Width;
    uint32_t m_Height;

    // GPU-visible SRV descriptors reserved per-frame (matches ImGui defaults and transient SRV needs)
    static constexpr UINT kSrvDescriptorCapacity = 1000;
    static constexpr UINT kSrvDescriptorsPerFrame = kSrvDescriptorCapacity / FrameCount;
    std::array<UINT, FrameCount> m_SrvDescriptorCursor{};
    std::array<D3D12_RESOURCE_STATES, FrameCount> m_BackBufferStates{};
    
    // DSV allocator
    UINT m_DsvDescriptorOffset = 1; // 0 is main depth buffer
};

} // namespace Henky3D
