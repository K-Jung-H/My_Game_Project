#pragma once
#include "Game_Resource.h"

class Texture : public Game_Resource
{
public:
	Texture();
	virtual ~Texture() = default;
	virtual bool LoadFromFile(std::string path, const RendererContext& ctx);
	virtual bool SaveToFile(const std::string& outputPath) const { return false; }

	void SetResource(ComPtr<ID3D12Resource> new_resource, const RendererContext& ctx);
	ID3D12Resource* GetResource() const { return mTexture.Get(); }

	UINT GetWidth() { return texture_width; }
	UINT GetHeight() { return texture_height; }

private:
	ComPtr<ID3D12Resource> mTexture;
	ComPtr<ID3D12Resource> mUploadBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE mGpuHandle = {};

	UINT texture_width = 0;
	UINT texture_height = 0;
};

