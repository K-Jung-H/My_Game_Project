#pragma once
#include "Game_Resource.h"
#include "Terrain/TerrainHeightField.h"

class TerrainResource : public Game_Resource
{
public:
    TerrainResource();
    virtual ~TerrainResource();

    virtual bool LoadFromFile(std::string path, const RendererContext& ctx) override;

    void CreateFromRawFile(std::string rawPath, float width, float depth, float maxHeight);

    UINT GetHeightMapID() const { return mHeightMapID; }
    const TerrainHeightField* GetHeightField() const { return mHeightField.get(); }

    //float GetWidth() const { return mHeightField ? mHeightField->GetRealWidth() : 0.0f; }
    //float GetDepth() const { return mHeightField ? mHeightField->GetRealDepth() : 0.0f; }
    //float GetMaxHeight() const { return mHeightField ? mHeightField->GetMaxHeight() : 0.0f; }

private:
	UINT mHeightMapID = Engine::INVALID_ID; // Height Map Texture Resource ID
    std::unique_ptr<TerrainHeightField> mHeightField;
};