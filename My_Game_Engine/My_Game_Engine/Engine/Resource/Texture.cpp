#pragma once
#include "Texture.h"
#include "DX_Graphics/Renderer.h"
#include "DX_Graphics/ResourceUtils.h"

static std::wstring ToWString(std::string_view str)
{
    if (str.empty()) return {};

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);

    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), wstr.data(), size_needed);

    return wstr;
}

Texture::Texture() : Game_Resource(ResourceType::Texture)
{
}

bool Texture::LoadFromFile(std::string path, const RendererContext& ctx)
{
    std::wstring wpath = ToWString(path);

    if (wpath.ends_with(L".dds"))
        mTexture = ResourceUtils::LoadDDSTexture(ctx, wpath, mUploadBuffer);
    else
        mTexture = ResourceUtils::LoadWICTexture(ctx, wpath, mUploadBuffer);

    if (!mTexture) return false;

    mSlot = ctx.resourceHeap->Allocate(HeapRegion::SRV_Static);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = mTexture->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = mTexture->GetDesc().MipLevels;

    ctx.device->CreateShaderResourceView(mTexture.Get(), &srvDesc, ctx.resourceHeap->GetCpuHandle(mSlot));

    mGpuHandle = ctx.resourceHeap->GetGpuHandle(mSlot);
    return true;
}