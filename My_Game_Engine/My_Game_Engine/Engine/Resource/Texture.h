#pragma once
#include "Game_Resource.h"

class Texture : public Game_Resource
{
public:
	Texture();
	virtual ~Texture() = default;
	virtual bool LoadFromFile(std::string_view path, const RendererContext& ctx);

	ID3D12Resource* GetResource() const { return mTexture.Get(); }

private:
	ComPtr<ID3D12Resource> mTexture;
	ComPtr<ID3D12Resource> mUploadBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE mGpuHandle = {};
};

