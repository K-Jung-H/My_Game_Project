#include "ResourceUtils.h"
#include "DDSTextureLoader12.h"
#include "WICTextureLoader12.h"
#include "Renderer.h"

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
    return CreateBufferResource(ctx, (void*)initData, static_cast<UINT>(byteSize), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, uploadBuffer);
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



ComPtr<ID3D12Resource> ResourceUtils::CreateBufferResource(const RendererContext& ctx, void* pData, UINT nBytes, D3D12_HEAP_TYPE heapType,
    D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES finalState, ComPtr<ID3D12Resource>& uploadBuffer)
{
    return CreateResource(ctx, pData, nBytes, D3D12_RESOURCE_DIMENSION_BUFFER, nBytes, 1, 1, 1, flags, DXGI_FORMAT_UNKNOWN, heapType, finalState, uploadBuffer);
}

ComPtr<ID3D12Resource> ResourceUtils::CreateBufferResourceEmpty(const RendererContext& ctx, UINT nBytes, D3D12_HEAP_TYPE heapType,
    D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES finalState)
{
    ComPtr<ID3D12Resource> dummyUpload;
    return CreateBufferResource(ctx, nullptr , nBytes, heapType, flags, finalState, dummyUpload);
}


ComPtr<ID3D12Resource> ResourceUtils::CreateTextureResource(const RendererContext& ctx, void* pData, UINT64 rowPitchBytes, UINT width, UINT height, UINT arraySize, UINT mipLevels,
    D3D12_RESOURCE_FLAGS flags, DXGI_FORMAT format, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES finalState, ComPtr<ID3D12Resource>& uploadBuffer)
{
    return CreateResource(ctx, pData, rowPitchBytes, D3D12_RESOURCE_DIMENSION_TEXTURE2D, width, height, arraySize, mipLevels, flags, format, heapType, finalState, uploadBuffer);
}

ComPtr<ID3D12Resource> ResourceUtils::CreateTexture2DArray(const RendererContext& ctx, UINT width, UINT height, DXGI_FORMAT format, UINT arraySize, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initialState)
{
    ComPtr<ID3D12Resource> dummyUpload;
	return CreateTextureResource(ctx, nullptr, 0, width, height, arraySize, 1, flags, format, D3D12_HEAP_TYPE_DEFAULT, initialState, dummyUpload);
}

ComPtr<ID3D12Resource> ResourceUtils::CreateTextureCubeArray(const RendererContext& ctx, UINT width, UINT height, DXGI_FORMAT format, UINT numCubes, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initialState)
{
	ComPtr<ID3D12Resource> dummyUpload;
	return CreateTextureResource(ctx, nullptr, 0, width, height, numCubes * 6, 1, flags, format, D3D12_HEAP_TYPE_DEFAULT, initialState, dummyUpload);
}



ComPtr<ID3D12Resource> ResourceUtils::CreateResource(const RendererContext& ctx, void* pData, UINT64 nBytes, D3D12_RESOURCE_DIMENSION dimension, UINT width, UINT height, UINT depthOrArraySize, UINT mipLevels,
    D3D12_RESOURCE_FLAGS flags, DXGI_FORMAT format, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES finalState, ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> resource;

    // === Heap Properties ===
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = heapType;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    // === Resource Desc ===
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = dimension;
    desc.Alignment = (dimension == D3D12_RESOURCE_DIMENSION_BUFFER) ? D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT : 0;
    desc.Width = (dimension == D3D12_RESOURCE_DIMENSION_BUFFER) ? nBytes : width;
    desc.Height = (dimension == D3D12_RESOURCE_DIMENSION_BUFFER) ? 1 : height;
    desc.DepthOrArraySize = (dimension == D3D12_RESOURCE_DIMENSION_BUFFER) ? 1 : depthOrArraySize;
    desc.MipLevels = (dimension == D3D12_RESOURCE_DIMENSION_BUFFER) ? 1 : mipLevels;
    desc.Format = (dimension == D3D12_RESOURCE_DIMENSION_BUFFER) ? DXGI_FORMAT_UNKNOWN : format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = (dimension == D3D12_RESOURCE_DIMENSION_BUFFER) ? D3D12_TEXTURE_LAYOUT_ROW_MAJOR : D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = flags;

    ID3D12Device* device = ctx.device;
    ID3D12GraphicsCommandList* cmdList = ctx.cmdList;

    switch (heapType)
    {
        // ========================
    case D3D12_HEAP_TYPE_DEFAULT:
    {

        D3D12_RESOURCE_STATES initState =
            (dimension == D3D12_RESOURCE_DIMENSION_BUFFER) ? D3D12_RESOURCE_STATE_COMMON : (pData ? D3D12_RESOURCE_STATE_COPY_DEST : finalState);

        HRESULT hr = device->CreateCommittedResource(&heapProps,
            D3D12_HEAP_FLAG_NONE, &desc, initState, nullptr, IID_PPV_ARGS(&resource));

        if (FAILED(hr))
            throw std::runtime_error("Failed to create default heap resource");

        if (pData)
        {
            heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

            hr = device->CreateCommittedResource(&heapProps,
                D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));

            if (FAILED(hr))
                throw std::runtime_error("Failed to create upload heap resource");

            if (dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
            {
                void* mapped = nullptr;
                uploadBuffer->Map(0, nullptr, &mapped);
                memcpy(mapped, pData, static_cast<size_t>(nBytes));
                uploadBuffer->Unmap(0, nullptr);

                CD3DX12_RESOURCE_BARRIER toCopy = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
                cmdList->ResourceBarrier(1, &toCopy);

                cmdList->CopyResource(resource.Get(), uploadBuffer.Get());

                CD3DX12_RESOURCE_BARRIER toFinal = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, finalState);
                cmdList->ResourceBarrier(1, &toFinal);
            }
            else // Texture ¡æ UpdateSubresources
            {
                D3D12_SUBRESOURCE_DATA subData = {};
                subData.pData = pData;
                subData.RowPitch = nBytes;
                subData.SlicePitch = nBytes;

                UpdateSubresources(cmdList, resource.Get(), uploadBuffer.Get(), 0, 0, 1, &subData);

                CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, finalState);
                cmdList->ResourceBarrier(1, &barrier);
            }
        }
        break;
    }

    // ========================
    case D3D12_HEAP_TYPE_UPLOAD:
    {
        HRESULT hr = device->CreateCommittedResource(&heapProps,
            D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));

        if (FAILED(hr))
            throw std::runtime_error("Failed to create upload resource");

        if (pData)
        {
            void* mapped = nullptr;
            resource->Map(0, nullptr, &mapped);
            memcpy(mapped, pData, static_cast<size_t>(nBytes));
            resource->Unmap(0, nullptr);
        }
        break;
    }

    // ========================
    case D3D12_HEAP_TYPE_READBACK:
    {
        HRESULT hr = device->CreateCommittedResource(&heapProps,
            D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resource));

        if (FAILED(hr))
            throw std::runtime_error("Failed to create readback resource");
        break;
    }
    }

    return resource;
}
