#pragma once

class TerrainHeightField
{
public:
    TerrainHeightField();
    ~TerrainHeightField();

    void BuildFromRawData(const std::vector<uint16_t>& rawData, UINT width, UINT height, float realWidth, float realDepth, float maxHeight);

    float GetHeight(float localX, float localZ) const;
    UINT GetWidthCount() const { return mWidthCount; }
    UINT GetHeightCount() const { return mHeightCount; }

private:
    std::vector<float> mHeightData;
    UINT mWidthCount = 0;
    UINT mHeightCount = 0;

    float mRealWidth = 0.0f;
    float mRealDepth = 0.0f;
    float mMaxHeight = 0.0f;
};