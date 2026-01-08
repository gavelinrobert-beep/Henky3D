// Minimal d3dx12.h implementation with required helpers
// This is a simplified version containing only the helpers used in the project
#ifndef __D3DX12_H__
#define __D3DX12_H__

#include <d3d12.h>

// CD3DX12_HEAP_PROPERTIES
struct CD3DX12_HEAP_PROPERTIES : public D3D12_HEAP_PROPERTIES
{
    CD3DX12_HEAP_PROPERTIES() = default;
    explicit CD3DX12_HEAP_PROPERTIES(const D3D12_HEAP_PROPERTIES &o) : D3D12_HEAP_PROPERTIES(o) {}
    CD3DX12_HEAP_PROPERTIES(
        D3D12_CPU_PAGE_PROPERTY cpuPageProperty,
        D3D12_MEMORY_POOL memoryPoolPreference,
        UINT creationNodeMask = 1,
        UINT nodeMask = 1)
    {
        Type = D3D12_HEAP_TYPE_CUSTOM;
        CPUPageProperty = cpuPageProperty;
        MemoryPoolPreference = memoryPoolPreference;
        CreationNodeMask = creationNodeMask;
        VisibleNodeMask = nodeMask;
    }
    explicit CD3DX12_HEAP_PROPERTIES(
        D3D12_HEAP_TYPE type,
        UINT creationNodeMask = 1,
        UINT nodeMask = 1)
    {
        Type = type;
        CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        CreationNodeMask = creationNodeMask;
        VisibleNodeMask = nodeMask;
    }
};

// CD3DX12_RESOURCE_BARRIER
struct CD3DX12_RESOURCE_BARRIER : public D3D12_RESOURCE_BARRIER
{
    CD3DX12_RESOURCE_BARRIER() = default;
    explicit CD3DX12_RESOURCE_BARRIER(const D3D12_RESOURCE_BARRIER &o) : D3D12_RESOURCE_BARRIER(o) {}
    
    static inline CD3DX12_RESOURCE_BARRIER Transition(
        ID3D12Resource* pResource,
        D3D12_RESOURCE_STATES stateBefore,
        D3D12_RESOURCE_STATES stateAfter,
        UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE)
    {
        CD3DX12_RESOURCE_BARRIER result = {};
        result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        result.Flags = flags;
        result.Transition.pResource = pResource;
        result.Transition.StateBefore = stateBefore;
        result.Transition.StateAfter = stateAfter;
        result.Transition.Subresource = subresource;
        return result;
    }
};

// Helper function to calculate required intermediate buffer size
inline UINT64 GetRequiredIntermediateSize(
    _In_ ID3D12Resource* pDestinationResource,
    _In_range_(0, D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
    _In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) UINT NumSubresources)
{
    D3D12_RESOURCE_DESC Desc = pDestinationResource->GetDesc();
    UINT64 RequiredSize = 0;
    
    ID3D12Device* pDevice = nullptr;
    pDestinationResource->GetDevice(IID_PPV_ARGS(&pDevice));
    pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, 0, nullptr, nullptr, nullptr, &RequiredSize);
    pDevice->Release();
    
    return RequiredSize;
}

// Helper to copy subresource data (forward declaration needed)
inline void MemcpySubresource(
    _In_ const D3D12_MEMCPY_DEST* pDest,
    _In_ const D3D12_SUBRESOURCE_DATA* pSrc,
    SIZE_T RowSizeInBytes,
    UINT NumRows,
    UINT NumSlices);

// Helper for UpdateSubresources
inline UINT64 UpdateSubresourcesHelper(
    _In_ ID3D12GraphicsCommandList* pCmdList,
    _In_ ID3D12Resource* pDestinationResource,
    _In_ ID3D12Resource* pIntermediate,
    _In_range_(0, D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
    _In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) UINT NumSubresources,
    UINT64 RequiredSize,
    _In_reads_(NumSubresources) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
    _In_reads_(NumSubresources) const UINT* pNumRows,
    _In_reads_(NumSubresources) const UINT64* pRowSizesInBytes,
    _In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData);

// Helper function to update subresources
inline UINT64 UpdateSubresources(
    _In_ ID3D12GraphicsCommandList* pCmdList,
    _In_ ID3D12Resource* pDestinationResource,
    _In_ ID3D12Resource* pIntermediate,
    UINT64 IntermediateOffset,
    _In_range_(0, D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
    _In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) UINT NumSubresources,
    _In_reads_(NumSubresources) D3D12_SUBRESOURCE_DATA* pSrcData)
{
    UINT64 RequiredSize = 0;
    UINT64 MemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * NumSubresources;
    if (MemToAlloc > SIZE_MAX)
    {
        return 0;
    }
    void* pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
    if (pMem == nullptr)
    {
        return 0;
    }
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
    UINT64* pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + NumSubresources);
    UINT* pNumRows = reinterpret_cast<UINT*>(pRowSizesInBytes + NumSubresources);

    D3D12_RESOURCE_DESC Desc = pDestinationResource->GetDesc();
    ID3D12Device* pDevice = nullptr;
    pDestinationResource->GetDevice(IID_PPV_ARGS(&pDevice));
    pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, IntermediateOffset, pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize);
    pDevice->Release();

    UINT64 Result = UpdateSubresourcesHelper(pCmdList, pDestinationResource, pIntermediate, FirstSubresource, NumSubresources, RequiredSize, pLayouts, pNumRows, pRowSizesInBytes, pSrcData);
    HeapFree(GetProcessHeap(), 0, pMem);
    return Result;
}

// Helper for UpdateSubresources (implementation)
inline UINT64 UpdateSubresourcesHelper(
    _In_ ID3D12GraphicsCommandList* pCmdList,
    _In_ ID3D12Resource* pDestinationResource,
    _In_ ID3D12Resource* pIntermediate,
    _In_range_(0, D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
    _In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) UINT NumSubresources,
    UINT64 RequiredSize,
    _In_reads_(NumSubresources) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
    _In_reads_(NumSubresources) const UINT* pNumRows,
    _In_reads_(NumSubresources) const UINT64* pRowSizesInBytes,
    _In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData)
{
    D3D12_RESOURCE_DESC IntermediateDesc = pIntermediate->GetDesc();
    D3D12_RESOURCE_DESC DestinationDesc = pDestinationResource->GetDesc();
    if (IntermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
        IntermediateDesc.Width < RequiredSize + pLayouts[0].Offset ||
        RequiredSize > static_cast<UINT64>(SIZE_MAX) ||
        (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
            (FirstSubresource != 0 || NumSubresources != 1)))
    {
        return 0;
    }

    BYTE* pData;
    HRESULT hr = pIntermediate->Map(0, nullptr, reinterpret_cast<void**>(&pData));
    if (FAILED(hr))
    {
        return 0;
    }

    for (UINT i = 0; i < NumSubresources; ++i)
    {
        if (pRowSizesInBytes[i] > static_cast<UINT64>(SIZE_MAX)) return 0;
        D3D12_MEMCPY_DEST DestData = { pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, SIZE_T(pLayouts[i].Footprint.RowPitch) * SIZE_T(pNumRows[i]) };
        MemcpySubresource(&DestData, &pSrcData[i], static_cast<SIZE_T>(pRowSizesInBytes[i]), pNumRows[i], pLayouts[i].Footprint.Depth);
    }
    pIntermediate->Unmap(0, nullptr);

    if (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        pCmdList->CopyBufferRegion(
            pDestinationResource, 0, pIntermediate, pLayouts[0].Offset, pLayouts[0].Footprint.Width);
    }
    else
    {
        for (UINT i = 0; i < NumSubresources; ++i)
        {
            D3D12_TEXTURE_COPY_LOCATION Dst = {};
            Dst.pResource = pDestinationResource;
            Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            Dst.SubresourceIndex = i + FirstSubresource;
            
            D3D12_TEXTURE_COPY_LOCATION Src = {};
            Src.pResource = pIntermediate;
            Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            Src.PlacedFootprint = pLayouts[i];
            
            pCmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
        }
    }
    return RequiredSize;
}

// Helper to copy subresource data
inline void MemcpySubresource(
    _In_ const D3D12_MEMCPY_DEST* pDest,
    _In_ const D3D12_SUBRESOURCE_DATA* pSrc,
    SIZE_T RowSizeInBytes,
    UINT NumRows,
    UINT NumSlices)
{
    for (UINT z = 0; z < NumSlices; ++z)
    {
        BYTE* pDestSlice = reinterpret_cast<BYTE*>(pDest->pData) + pDest->SlicePitch * z;
        const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(pSrc->pData) + pSrc->SlicePitch * LONG_PTR(z);
        for (UINT y = 0; y < NumRows; ++y)
        {
            memcpy(pDestSlice + pDest->RowPitch * y,
                   pSrcSlice + pSrc->RowPitch * LONG_PTR(y),
                   RowSizeInBytes);
        }
    }
}

#endif // __D3DX12_H__
