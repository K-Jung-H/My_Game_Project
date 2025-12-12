#pragma once
#include "Game_Resource.h"
#include "Terrain/TerrainHeightField.h"

class TerrainResource : public Game_Resource
{
public:
    TerrainResource();
    virtual ~TerrainResource();

    virtual bool LoadFromFile(std::string path, const RendererContext& ctx) override;
    virtual bool SaveToFile(const std::string& path) const;

    UINT GetHeightMapID() const { return mHeightMapID; }
    const TerrainHeightField* GetHeightField() const { return mHeightField.get(); }

private:
	UINT mHeightMapID = Engine::INVALID_ID; // Height Map Texture Resource ID
    std::unique_ptr<TerrainHeightField> mHeightField;
};