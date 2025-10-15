#pragma once

struct RendererContext;

namespace ResourceUtils
{
    ComPtr<ID3D12Resource> CreateDefaultBuffer(const RendererContext& ctx, const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer);
    ComPtr<ID3D12Resource> LoadDDSTexture(const RendererContext& ctx, const std::wstring& filename, ComPtr<ID3D12Resource>& uploadBuffer);
    ComPtr<ID3D12Resource> LoadWICTexture(const RendererContext& ctx, const std::wstring& filename, ComPtr<ID3D12Resource>& uploadBuffer);


    ComPtr<ID3D12Resource> CreateResource(const RendererContext& ctx, void* pData, UINT64 nBytes, D3D12_RESOURCE_DIMENSION dimension, UINT width, UINT height, UINT depthOrArraySize, UINT mipLevels, D3D12_RESOURCE_FLAGS flags, DXGI_FORMAT format, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES finalState, ComPtr<ID3D12Resource>& uploadBuffer);
    ComPtr<ID3D12Resource> CreateBufferResource(const RendererContext& ctx, void* pData, UINT nBytes, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES finalState, ComPtr<ID3D12Resource>& uploadBuffer);
	ComPtr<ID3D12Resource> CreateBufferResourceEmpty(const RendererContext& ctx, UINT nBytes, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES finalState); // Dummy upload
    ComPtr<ID3D12Resource> CreateTextureResource(const RendererContext& ctx, void* pData, UINT64 rowPitchBytes, UINT width, UINT height, UINT arraySize, UINT mipLevels, D3D12_RESOURCE_FLAGS flags, DXGI_FORMAT format, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES finalState, ComPtr<ID3D12Resource>& uploadBuffer);


}