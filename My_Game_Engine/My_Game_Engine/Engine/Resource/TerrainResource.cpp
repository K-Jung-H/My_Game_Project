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

bool TerrainResource::LoadFromFile(std::string path, const RendererContext& ctx)
{
    return false;
}

void TerrainResource::CreateFromRawFile(std::string rawPath, float width, float depth, float maxHeight)
{
    std::ifstream file(rawPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return;

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    UINT totalPixels = static_cast<UINT>(fileSize / 2);
    UINT resolution = static_cast<UINT>(std::sqrt(totalPixels));

    std::vector<uint16_t> rawData(totalPixels);
    file.read(reinterpret_cast<char*>(rawData.data()), fileSize);
    file.close();

    mHeightField = std::make_unique<TerrainHeightField>();
    mHeightField->BuildFromRawData(rawData, resolution, resolution, width, depth, maxHeight);

    auto rc = GameEngine::Get().Get_UploadContext();
    ComPtr<ID3D12Resource> uploadBuffer;

    ComPtr<ID3D12Resource> texResource = 
        ResourceUtils::CreateTextureFromMemory(rc, rawData.data(), resolution, resolution, DXGI_FORMAT_R16_UNORM, 2, uploadBuffer);

    if (texResource)
    {
        auto texture = std::make_shared<Texture>();
        texture->SetResource(texResource, rc);
        texture->SetPath(rawPath);
        texture->SetAlias("Terrain_HeightMap");
    }
}
