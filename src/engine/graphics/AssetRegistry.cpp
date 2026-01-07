#include "AssetRegistry.h"
#include <d3dx12.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <stdexcept>
#include <algorithm>

#pragma comment(lib, "windowscodecs.lib")

using Microsoft::WRL::ComPtr;

namespace Henky3D {

AssetRegistry::AssetRegistry(GraphicsDevice* device)
    : m_Device(device) {
}

AssetRegistry::~AssetRegistry() {
}

void AssetRegistry::InitializeDefaults() {
    // Create default white texture (1x1 RGBA)
    uint8_t whitePixel[] = { 255, 255, 255, 255 };
    m_DefaultWhiteTexture = CreateDefaultTexture("DefaultWhite", 1, 1, whitePixel, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

    // Create default normal texture (1x1, pointing up: 128, 128, 255 in RGB = 0.5, 0.5, 1.0 normalized)
    uint8_t normalPixel[] = { 128, 128, 255, 255 };
    m_DefaultNormalTexture = CreateDefaultTexture("DefaultNormal", 1, 1, normalPixel, DXGI_FORMAT_R8G8B8A8_UNORM);

    // Create default roughness/metalness texture (1x1, R=0 metalness, G=128 mid-roughness)
    uint8_t rmPixel[] = { 0, 128, 0, 255 };
    m_DefaultRoughnessMetalnessTexture = CreateDefaultTexture("DefaultRM", 1, 1, rmPixel, DXGI_FORMAT_R8G8B8A8_UNORM);
}

TextureHandle AssetRegistry::CreateDefaultTexture(const std::string& name, uint32_t width, uint32_t height,
                                                  const uint8_t* data, DXGI_FORMAT format) {
    auto texture = std::make_unique<TextureAsset>();
    texture->Path = std::wstring(name.begin(), name.end());
    texture->Width = width;
    texture->Height = height;
    texture->Format = format;
    texture->IsDefault = true;

    // Create texture resource
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = format;
    texDesc.SampleDesc.Count = 1;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    if (FAILED(m_Device->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&texture->Resource)))) {
        throw std::runtime_error("Failed to create default texture resource");
    }

    // Upload data via upload buffer
    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture->Resource.Get(), 0, 1);
    auto uploadBuffer = m_Device->CreateUploadBuffer(uploadBufferSize);

    D3D12_SUBRESOURCE_DATA subresourceData = {};
    subresourceData.pData = data;
    subresourceData.RowPitch = width * 4; // RGBA = 4 bytes per pixel
    subresourceData.SlicePitch = subresourceData.RowPitch * height;

    auto commandList = m_Device->GetCommandList();
    UpdateSubresources(commandList, texture->Resource.Get(), uploadBuffer.Resource.Get(), 
                      0, 0, 1, &subresourceData);

    // Transition to shader resource
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        texture->Resource.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &barrier);

    // Create SRV
    auto srvHandle = m_Device->AllocateSrvDescriptor(1);
    texture->SRV = srvHandle.CPUHandle;
    texture->SRVGPU = srvHandle.GPUHandle;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    m_Device->GetDevice()->CreateShaderResourceView(texture->Resource.Get(), &srvDesc, texture->SRV);

    // Add to registry
    TextureHandle handle;
    handle.Index = static_cast<uint32_t>(m_Textures.size());
    m_Textures.push_back(std::move(texture));

    return handle;
}

TextureHandle AssetRegistry::LoadTexture(const std::wstring& path) {
    // Check cache
    auto it = m_TextureCache.find(path);
    if (it != m_TextureCache.end()) {
        return it->second;
    }

    // Try WIC loader first
    TextureHandle handle = LoadTextureWIC(path);
    
    // If WIC fails, try DDS
    if (!handle.IsValid()) {
        handle = LoadTextureDDS(path);
    }

    // If both fail, return default white
    if (!handle.IsValid()) {
        return m_DefaultWhiteTexture;
    }

    m_TextureCache[path] = handle;
    return handle;
}

TextureHandle AssetRegistry::LoadTextureWIC(const std::wstring& path) {
    // Initialize COM
    CoInitialize(nullptr);

    ComPtr<IWICImagingFactory> wicFactory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory)))) {
        return TextureHandle{};
    }

    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(wicFactory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, 
                                                      WICDecodeMetadataCacheOnDemand, &decoder))) {
        return TextureHandle{};
    }

    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, &frame))) {
        return TextureHandle{};
    }

    UINT width, height;
    if (FAILED(frame->GetSize(&width, &height))) {
        return TextureHandle{};
    }

    // Convert to RGBA format
    ComPtr<IWICFormatConverter> converter;
    if (FAILED(wicFactory->CreateFormatConverter(&converter))) {
        return TextureHandle{};
    }

    if (FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
                                      WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeMedianCut))) {
        return TextureHandle{};
    }

    // Read pixel data
    UINT stride = width * 4;
    UINT bufferSize = stride * height;
    std::vector<uint8_t> pixelData(bufferSize);
    if (FAILED(converter->CopyPixels(nullptr, stride, bufferSize, pixelData.data()))) {
        return TextureHandle{};
    }

    // Create texture
    auto texture = std::make_unique<TextureAsset>();
    texture->Path = path;
    texture->Width = width;
    texture->Height = height;
    
    // Determine format based on filename (simple heuristic)
    std::wstring lowerPath = path;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
    if (lowerPath.find(L"normal") != std::wstring::npos || lowerPath.find(L"_n.") != std::wstring::npos) {
        texture->Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Linear for normal maps
    } else if (lowerPath.find(L"roughness") != std::wstring::npos || lowerPath.find(L"metalness") != std::wstring::npos ||
               lowerPath.find(L"_rm.") != std::wstring::npos || lowerPath.find(L"_mr.") != std::wstring::npos) {
        texture->Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Linear for RM maps
    } else {
        texture->Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // sRGB for base color
    }

    // Create texture resource
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = texture->Format;
    texDesc.SampleDesc.Count = 1;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    if (FAILED(m_Device->GetDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&texture->Resource)))) {
        return TextureHandle{};
    }

    // Upload data
    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture->Resource.Get(), 0, 1);
    auto uploadBuffer = m_Device->CreateUploadBuffer(uploadBufferSize);

    D3D12_SUBRESOURCE_DATA subresourceData = {};
    subresourceData.pData = pixelData.data();
    subresourceData.RowPitch = stride;
    subresourceData.SlicePitch = bufferSize;

    auto commandList = m_Device->GetCommandList();
    UpdateSubresources(commandList, texture->Resource.Get(), uploadBuffer.Resource.Get(),
                      0, 0, 1, &subresourceData);

    // Transition to shader resource
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        texture->Resource.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &barrier);

    // Create SRV
    auto srvHandle = m_Device->AllocateSrvDescriptor(1);
    texture->SRV = srvHandle.CPUHandle;
    texture->SRVGPU = srvHandle.GPUHandle;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texture->Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    m_Device->GetDevice()->CreateShaderResourceView(texture->Resource.Get(), &srvDesc, texture->SRV);

    // Add to registry
    TextureHandle handle;
    handle.Index = static_cast<uint32_t>(m_Textures.size());
    m_Textures.push_back(std::move(texture));

    return handle;
}

TextureHandle AssetRegistry::LoadTextureDDS(const std::wstring& path) {
    // DDS loading not implemented - return invalid handle
    // Could use DirectXTex library for full DDS support
    return TextureHandle{};
}

const TextureAsset* AssetRegistry::GetTexture(TextureHandle handle) const {
    if (!handle.IsValid() || handle.Index >= m_Textures.size()) {
        return nullptr;
    }
    return m_Textures[handle.Index].get();
}

uint32_t AssetRegistry::CreateMaterial(const MaterialAsset& material) {
    uint32_t index = static_cast<uint32_t>(m_Materials.size());
    m_Materials.push_back(material);
    return index;
}

const MaterialAsset* AssetRegistry::GetMaterial(uint32_t index) const {
    if (index >= m_Materials.size()) {
        return nullptr;
    }
    return &m_Materials[index];
}

MaterialAsset* AssetRegistry::GetMaterialMutable(uint32_t index) {
    if (index >= m_Materials.size()) {
        return nullptr;
    }
    return &m_Materials[index];
}

} // namespace Henky3D
