#pragma once

class TerrainHeightField
{
public:
    TerrainHeightField();
    ~TerrainHeightField();

    void BuildFromRawData(const std::vector<uint16_t>& rawData, UINT resolution);

    float GetHeight(float localX, float localZ) const;

    UINT GetWidthCount() const { return mWidthCount; }
    UINT GetHeightCount() const { return mHeightCount; }

private:
    std::vector<float> mHeightData;
    UINT mWidthCount = 0;
    UINT mHeightCount = 0;
};