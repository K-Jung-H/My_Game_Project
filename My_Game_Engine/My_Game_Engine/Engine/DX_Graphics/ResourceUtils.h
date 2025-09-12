#pragma once
#include "Renderer.h"

namespace ResourceUtils
{
    ComPtr<ID3D12Resource> CreateDefaultBuffer(const RendererContext& ctx, const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer);
    ComPtr<ID3D12Resource> LoadDDSTexture(const RendererContext& ctx, const std::wstring& filename, ComPtr<ID3D12Resource>& uploadBuffer);
    ComPtr<ID3D12Resource> LoadWICTexture(const RendererContext& ctx, const std::wstring& filename, ComPtr<ID3D12Resource>& uploadBuffer);
}