#include "TerrainHeightField.h"
#include "DXMathUtils.h"

TerrainHeightField::TerrainHeightField()
{
}

TerrainHeightField::~TerrainHeightField()
{
}

void TerrainHeightField::BuildFromRawData(const std::vector<uint16_t>& rawData, UINT resolution)
{
    mWidthCount = resolution;
    mHeightCount = resolution;

    size_t requiredPixels = static_cast<size_t>(mWidthCount * mHeightCount);

    if (rawData.size() < requiredPixels)
        return;


    mHeightData.resize(requiredPixels);
    float invMax = 1.0f / 65535.0f;

    for (size_t i = 0; i < requiredPixels; ++i)
        mHeightData[i] = rawData[i] * invMax;

}


float TerrainHeightField::GetHeight(float u, float v) const
{
    if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f)
        return 0.0f;

    float gridX = u * (mWidthCount - 1);
    float gridZ = v * (mHeightCount - 1);

    int x = (int)gridX;
    int z = (int)gridZ;

    if (x >= (int)mWidthCount - 1) x = mWidthCount - 2;
    if (z >= (int)mHeightCount - 1) z = mHeightCount - 2;

    float dx = gridX - x;
    float dz = gridZ - z;

    float h00 = mHeightData[z * mWidthCount + x];
    float h10 = mHeightData[z * mWidthCount + (x + 1)];
    float h01 = mHeightData[(z + 1) * mWidthCount + x];
    float h11 = mHeightData[(z + 1) * mWidthCount + (x + 1)];

    float top = Math::Lerp(h00, h10, dx);
    float bot = Math::Lerp(h01, h11, dx);

    return Math::Lerp(top, bot, dz);
}