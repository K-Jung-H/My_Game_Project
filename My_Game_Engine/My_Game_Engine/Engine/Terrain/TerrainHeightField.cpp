#include "TerrainHeightField.h"
#include "DXMathUtils.h"

TerrainHeightField::TerrainHeightField()
{
}

TerrainHeightField::~TerrainHeightField()
{
}

void TerrainHeightField::BuildFromRawData(const std::vector<uint16_t>& rawData, UINT width, UINT height, float realWidth, float realDepth, float maxHeight)
{
    mRealWidth = realWidth;
    mRealDepth = realDepth;
    mMaxHeight = maxHeight;
    mWidthCount = width;
    mHeightCount = height;

    size_t totalPixels = rawData.size();
    if (totalPixels != static_cast<size_t>(mWidthCount * mHeightCount))
        return;

    mHeightData.resize(totalPixels);
    float invMax = 1.0f / 65535.0f;

    for (size_t i = 0; i < totalPixels; ++i)
    {
        float normalized = rawData[i] * invMax;
        mHeightData[i] = normalized * mMaxHeight;
    }
}

float TerrainHeightField::GetHeight(float localX, float localZ) const
{
    if (localX < 0 || localX > mRealWidth || localZ < 0 || localZ > mRealDepth)
        return -FLT_MAX;

    float gridX = (localX / mRealWidth) * (mWidthCount - 1);
    float gridZ = (localZ / mRealDepth) * (mHeightCount - 1);

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