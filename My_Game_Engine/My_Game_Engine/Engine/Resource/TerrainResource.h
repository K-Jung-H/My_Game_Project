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

    UINT GetHeightMapTextureResourceID() const { return mHeightMapTextureResourceID; }
    const TerrainHeightField* GetHeightField() const { return mHeightField.get(); }

private:
	UINT mHeightMapTextureResourceID = Engine::INVALID_ID; // Height Map Texture Resource ID
    std::unique_ptr<TerrainHeightField> mHeightField;
};