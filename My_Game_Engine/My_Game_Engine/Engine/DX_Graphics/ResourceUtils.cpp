#include "pch.h"
#include "ResourceUtils.h"
#include "DDSTextureLoader12.h"
#include "WICTextureLoader12.h"

using namespace DirectX;

static void LogIfFailed(HRESULT hr, const char* msg)
{
    if (FAILED(hr))
    {
        char buffer[256];
        sprintf_s(buffer, "[DX12][Error] %s (HRESULT=0x%08X)\n", msg, (unsigned)hr);
        OutputDebugStringA(buffer);
    }
}

// =====================================================
// CreateDefaultBuffer
// =====================================================
ComPtr<ID3D12Resource> ResourceUtils::CreateDefaultBuffer(const RendererContext& ctx, const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);

    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);


    HRESULT hr = ctx.device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(defaultBuffer.GetAddressOf()));

    if (FAILED(hr)) 
    { 
        LogIfFailed(hr, "CreateDefaultBuffer: default heap");
        return nullptr; 
    }


    hr = ctx.device->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE,
        &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(uploadBuffer.GetAddressOf()));

    if (FAILED(hr))
    {
        LogIfFailed(hr, "CreateDefaultBuffer: upload heap");
        return nullptr;
    }

    if (initData) 
    {
        D3D12_SUBRESOURCE_DATA subData = {};
        subData.pData = initData;
        subData.RowPitch = byteSize;
        subData.SlicePitch = byteSize;

        UpdateSubresources<1>(ctx.cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subData);

        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

        ctx.cmdList->ResourceBarrier(1, &barrier);
    }

    return defaultBuffer;
}

// =====================================================
// LoadDDSTexture
// =====================================================
ComPtr<ID3D12Resource> ResourceUtils::LoadDDSTexture(const RendererContext& ctx, const std::wstring& filename, ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> texture;
    std::unique_ptr<uint8_t[]> ddsData;
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;

    HRESULT hr = DirectX::LoadDDSTextureFromFile(ctx.device, filename.c_str(), texture.GetAddressOf(), ddsData, subresources);
    if (FAILED(hr)) 
    {
        LogIfFailed(hr, "LoadDDSTextureFromFile");
        return nullptr;
    }

    const UINT numSubresources = (UINT)subresources.size();
    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, numSubresources);
    
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    hr = ctx.device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, 
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(uploadBuffer.GetAddressOf()));

    if (FAILED(hr)) 
    {
        LogIfFailed(hr, "LoadDDSTexture: upload heap");
        return nullptr;
    }

    UpdateSubresources(ctx.cmdList, texture.Get(), uploadBuffer.Get(), 0, 0, numSubresources, subresources.data());

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    ctx.cmdList->ResourceBarrier(1, &barrier);

    return texture;
}

// =====================================================
// LoadWICTexture
// =====================================================
ComPtr<ID3D12Resource> ResourceUtils::LoadWICTexture(const RendererContext& ctx, const std::wstring& filename, ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> texture;
    std::unique_ptr<uint8_t[]> decodedData;
    D3D12_SUBRESOURCE_DATA subresource;

    HRESULT hr = DirectX::LoadWICTextureFromFile(ctx.device, filename.c_str(), texture.GetAddressOf(), decodedData, subresource);

    if (FAILED(hr)) 
    { 
        LogIfFailed(hr, "LoadWICTextureFromFile"); 
        return nullptr; 
    }

    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, 1);


    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    hr = ctx.device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(uploadBuffer.GetAddressOf()));

    if (FAILED(hr))
    {
        LogIfFailed(hr, "LoadWICTexture: upload heap");
        return nullptr;
    }

    UpdateSubresources(ctx.cmdList, texture.Get(), uploadBuffer.Get(), 0, 0, 1, &subresource);

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    ctx.cmdList->ResourceBarrier(1, &barrier);

    return texture;
}
