#include "TerrainResource.h"
#include "GameEngine.h"
#include "ResourceSystem.h"
#include "DX_Graphics/ResourceUtils.h"

TerrainResource::TerrainResource()
    : Game_Resource(ResourceType::TerrainData)
{
}

TerrainResource::~TerrainResource()
{
}

bool TerrainResource::SaveToFile(const std::string& path) const
{
    return false;
}

bool TerrainResource::LoadFromFile(std::string rawPath, const RendererContext& ctx)
{
    std::ifstream file(rawPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    int bytesPerPixel = 0;
    UINT resolution = 0;
    const int candidates[] = { 2, 4, 1 };

    for (int bpp : candidates)
    {
        if (fileSize % bpp == 0)
        {
            size_t pixelCount = fileSize / bpp;
            double root = std::sqrt(pixelCount);
            UINT res = static_cast<UINT>(root);

            if (res * res == pixelCount)
            {
                bytesPerPixel = bpp;
                resolution = res;
                break;
            }
        }
    }

    if (bytesPerPixel == 0)
    {
        file.close();
		OutputDebugStringA(("[TerrainResource] Unsupported heightmap format or corrupted file: " + rawPath + "\n").c_str());
        return false;
    }

    std::vector<uint8_t> fileBuffer(fileSize);
    file.read(reinterpret_cast<char*>(fileBuffer.data()), fileSize);
    file.close();

    std::vector<uint16_t> normalizedData(resolution * resolution);
    size_t pixelCount = normalizedData.size();

    if (bytesPerPixel == 2) 
    {
        memcpy(normalizedData.data(), fileBuffer.data(), fileSize);
    }
    else if (bytesPerPixel == 4) 
    {
        const float* src = reinterpret_cast<const float*>(fileBuffer.data());
        for (size_t i = 0; i < pixelCount; ++i)
        {
            normalizedData[i] = static_cast<uint16_t>(std::clamp(src[i], 0.0f, 1.0f) * 65535.0f);
        }
    }
    else if (bytesPerPixel == 1) 
    {
        const uint8_t* src = reinterpret_cast<const uint8_t*>(fileBuffer.data());
        for (size_t i = 0; i < pixelCount; ++i)
        {
            normalizedData[i] = static_cast<uint16_t>(src[i]) * 257; 
        }
    }

    mHeightField = std::make_unique<TerrainHeightField>();
    mHeightField->BuildFromRawData(normalizedData, resolution);

    auto rc = GameEngine::Get().Get_UploadContext();

    ComPtr<ID3D12Resource> uploadBuffer;
    ComPtr<ID3D12Resource> texResource = ResourceUtils::CreateTextureFromMemory(rc, normalizedData.data(), resolution, resolution, DXGI_FORMAT_R16_UNORM, 2, uploadBuffer);


    if (texResource)
    {
        auto texture = std::make_shared<Texture>();
        texture->SetResource(texResource, rc, uploadBuffer);
        texture->SetState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        auto rsc = GameEngine::Get().GetResourceSystem();
        rsc->RegisterResource(texture);

        texture->SetPath(rawPath);
        texture->SetAlias(rawPath + "_HeightMap");

        mHeightMapTextureResourceID = texture->GetId();
        return true;
    }

    return false;
}