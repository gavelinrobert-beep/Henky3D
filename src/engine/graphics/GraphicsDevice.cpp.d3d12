#include "GraphicsDevice.h"
#include "../core/Window.h"
#include <stdexcept>
#include <atomic>
#include <sstream>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

namespace Henky3D {

GraphicsDevice::GraphicsDevice(Window* window)
    : m_FrameIndex(0), m_Width(window->GetWidth()), m_Height(window->GetHeight()) {
    
#ifdef _DEBUG
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
    }
#endif

    CreateDeviceAndQueues();
    CreateSwapChain(window);
    CreateDescriptorHeaps();
    CreateRenderTargets();
    CreateDepthStencil(m_Width, m_Height);
    CreateCommandObjects();
    CreateFences();
}

GraphicsDevice::~GraphicsDevice() {
    WaitForGPU();
    if (m_FenceEvent) {
        CloseHandle(m_FenceEvent);
    }
}

void GraphicsDevice::CreateDeviceAndQueues() {
    UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_Factory)))) {
        throw std::runtime_error("Failed to create DXGI factory");
    }

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; m_Factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_Device)))) {
            break;
        }
    }

    if (!m_Device) {
        throw std::runtime_error("Failed to create D3D12 device");
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    
    if (FAILED(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)))) {
        throw std::runtime_error("Failed to create command queue");
    }

    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    if (FAILED(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_ComputeQueue)))) {
        throw std::runtime_error("Failed to create compute queue");
    }

    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    if (FAILED(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CopyQueue)))) {
        throw std::runtime_error("Failed to create copy queue");
    }
}

void GraphicsDevice::CreateSwapChain(Window* window) {
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = m_Width;
    swapChainDesc.Height = m_Height;
    swapChainDesc.Format = BackBufferFormat;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    ComPtr<IDXGISwapChain1> swapChain;
    if (FAILED(m_Factory->CreateSwapChainForHwnd(
        m_CommandQueue.Get(),
        window->GetHandle(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain))) {
        throw std::runtime_error("Failed to create swap chain");
    }

    if (FAILED(swapChain.As(&m_SwapChain))) {
        throw std::runtime_error("Failed to get IDXGISwapChain3 interface");
    }

    m_Factory->MakeWindowAssociation(window->GetHandle(), DXGI_MWA_NO_ALT_ENTER);
}

void GraphicsDevice::CreateDescriptorHeaps() {
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap)))) {
        throw std::runtime_error("Failed to create RTV descriptor heap");
    }

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 10; // Support main depth buffer + shadow maps
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(m_Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap)))) {
        throw std::runtime_error("Failed to create DSV descriptor heap");
    }

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = kSrvDescriptorCapacity;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(m_Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SRVHeap)))) {
        throw std::runtime_error("Failed to create SRV descriptor heap");
    }

    m_RTVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_DSVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_SRVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void GraphicsDevice::CreateRenderTargets() {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
    
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    
    for (UINT i = 0; i < FrameCount; i++) {
        m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_RenderTargets[i]));
        m_Device->CreateRenderTargetView(m_RenderTargets[i].Get(), &rtvDesc, rtvHandle);
        rtvHandle.ptr += m_RTVDescriptorSize;
        m_BackBufferStates[i] = D3D12_RESOURCE_STATE_PRESENT;
    }
}

void GraphicsDevice::CreateDepthStencil(uint32_t width, uint32_t height) {
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Width = width;
    resourceDesc.Height = height;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DepthStencilFormat;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DepthStencilFormat;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    if (FAILED(m_Device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&m_DepthStencil)))) {
        throw std::runtime_error("Failed to create depth stencil buffer");
    }

    m_Device->CreateDepthStencilView(m_DepthStencil.Get(), nullptr, GetDSV());
}

void GraphicsDevice::CreateCommandObjects() {
    for (UINT i = 0; i < FrameCount; i++) {
        if (FAILED(m_Device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&m_CommandAllocators[i])))) {
            throw std::runtime_error("Failed to create command allocator");
        }
    }

    if (FAILED(m_Device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_CommandAllocators[0].Get(),
        nullptr,
        IID_PPV_ARGS(&m_CommandList)))) {
        throw std::runtime_error("Failed to create command list");
    }
    
    m_CommandList->Close();
}

void GraphicsDevice::CreateFences() {
    if (FAILED(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)))) {
        throw std::runtime_error("Failed to create fence");
    }
    m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    
    for (UINT i = 0; i < FrameCount; i++) {
        m_FenceValues[i] = 0;
    }
}

void GraphicsDevice::BeginFrame() {
    m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();
    
    m_CommandAllocators[m_FrameIndex]->Reset();
    m_CommandList->Reset(m_CommandAllocators[m_FrameIndex].Get(), nullptr);
    m_SrvDescriptorCursor[m_FrameIndex] = 0;
}

void GraphicsDevice::EndFrame() {
    m_CommandList->Close();
    
    ID3D12CommandList* commandLists[] = { m_CommandList.Get() };
    m_CommandQueue->ExecuteCommandLists(1, commandLists);
    
    m_SwapChain->Present(1, 0);
    
    const UINT64 currentFenceValue = m_FenceValues[m_FrameIndex];
    m_CommandQueue->Signal(m_Fence.Get(), currentFenceValue);
    
    m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();
    
    if (m_Fence->GetCompletedValue() < m_FenceValues[m_FrameIndex]) {
        m_Fence->SetEventOnCompletion(m_FenceValues[m_FrameIndex], m_FenceEvent);
        WaitForSingleObject(m_FenceEvent, INFINITE);
    }
    
    m_FenceValues[m_FrameIndex] = currentFenceValue + 1;
}

void GraphicsDevice::WaitForGPU() {
    const UINT64 fenceValue = m_FenceValues[m_FrameIndex];
    m_CommandQueue->Signal(m_Fence.Get(), fenceValue);
    m_FenceValues[m_FrameIndex]++;
    
    if (m_Fence->GetCompletedValue() < fenceValue) {
        m_Fence->SetEventOnCompletion(fenceValue, m_FenceEvent);
        WaitForSingleObject(m_FenceEvent, INFINITE);
    }
}

void GraphicsDevice::ResizeBuffers(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;
    
    WaitForGPU();
    
    for (UINT i = 0; i < FrameCount; i++) {
        m_RenderTargets[i].Reset();
    }
    m_DepthStencil.Reset();
    
    m_SwapChain->ResizeBuffers(FrameCount, width, height, BackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
    
    m_Width = width;
    m_Height = height;
    
    CreateRenderTargets();
    CreateDepthStencil(width, height);
    for (UINT i = 0; i < FrameCount; ++i) {
        m_BackBufferStates[i] = D3D12_RESOURCE_STATE_PRESENT;
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicsDevice::GetCurrentRTV() const {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += m_SwapChain->GetCurrentBackBufferIndex() * m_RTVDescriptorSize;
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicsDevice::GetDSV() const {
    return m_DSVHeap->GetCPUDescriptorHandleForHeapStart();
}

UploadBuffer GraphicsDevice::CreateUploadBuffer(UINT64 size) {
    UploadBuffer buffer;
    buffer.Size = size;
    
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    
    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = size;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    
    m_Device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&buffer.Resource));
    
    D3D12_RANGE readRange = { 0, 0 };
    buffer.Resource->Map(0, &readRange, &buffer.MappedData);
    
    return buffer;
}

UINT GraphicsDevice::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type) {
    static std::atomic<UINT> srvOffset = 0;
    if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
        return srvOffset++;
    }
    return 0;
}

DescriptorHandle GraphicsDevice::AllocateSrvDescriptor(UINT count) {
    constexpr UINT frameCapacity = kSrvDescriptorsPerFrame;
    if (count == 0 || count > frameCapacity) {
        std::ostringstream oss;
        oss << "Invalid descriptor count; must be between 1 and " << frameCapacity << " (requested " << count << ")";
        throw std::runtime_error(oss.str());
    }

    UINT frameCursor = m_SrvDescriptorCursor[m_FrameIndex];
    if (count > frameCapacity - frameCursor) {
        std::ostringstream oss;
        oss << "SRV descriptor heap exhausted for the current frame (requested " << count
            << ", available " << (frameCapacity - frameCursor) << ")";
        throw std::runtime_error(oss.str());
    }

    const UINT baseOffset = m_FrameIndex * frameCapacity + frameCursor;
    m_SrvDescriptorCursor[m_FrameIndex] = frameCursor + count;

    DescriptorHandle handle{};
    handle.CPUHandle = m_SRVHeap->GetCPUDescriptorHandleForHeapStart();
    handle.CPUHandle.ptr += static_cast<SIZE_T>(baseOffset) * m_SRVDescriptorSize;

    handle.GPUHandle = m_SRVHeap->GetGPUDescriptorHandleForHeapStart();
    handle.GPUHandle.ptr += static_cast<UINT64>(baseOffset) * m_SRVDescriptorSize;
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicsDevice::AllocateDsvDescriptor() {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(m_DsvDescriptorOffset) * m_DSVDescriptorSize;
    m_DsvDescriptorOffset++;
    return handle;
}

void GraphicsDevice::TransitionBackBuffer(ID3D12GraphicsCommandList* commandList, UINT bufferIndex, D3D12_RESOURCE_STATES newState) {
    const auto currentState = m_BackBufferStates[bufferIndex];
    if (currentState == newState) {
        return;
    }

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_RenderTargets[bufferIndex].Get();
    barrier.Transition.StateBefore = currentState;
    barrier.Transition.StateAfter = newState;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);
    m_BackBufferStates[bufferIndex] = newState;
}

void GraphicsDevice::TransitionBackBufferToRenderTarget(ID3D12GraphicsCommandList* commandList) {
    TransitionBackBuffer(commandList, GetBackBufferIndex(), D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void GraphicsDevice::TransitionBackBufferToPresent(ID3D12GraphicsCommandList* commandList) {
    TransitionBackBuffer(commandList, GetBackBufferIndex(), D3D12_RESOURCE_STATE_PRESENT);
}

} // namespace Henky3D
