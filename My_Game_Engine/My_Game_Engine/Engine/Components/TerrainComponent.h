#pragma once
#include "Core/Component.h"
#include "Terrain/TerrainQuadTree.h"
#include "Resource/TerrainResource.h"


class Mesh;
class Texture;
class Material;
class TransformComponent;

class TerrainComponent : public Component
{
public:
    //virtual rapidjson::Value ToJSON(rapidjson::Document::AllocatorType& alloc) const;
    //virtual void FromJSON(const rapidjson::Value& val);

public:
    static constexpr Component_Type Type = Component_Type::Terrain;
    Component_Type GetType() const override { return Type; }

public:
    TerrainComponent();
    virtual ~TerrainComponent();

    void SetTransform(std::weak_ptr<TransformComponent> tf);

    void SetTerrain(UINT TerrainResourceID);

    void SetTerrain_Size(float width, float depth, float maxHeight);
    void SetTerrain_Width(float width);
    void SetTerrain_Depth(float depth);
    void SetTerrain_MaxHeight(float maxHeight);
    void SetTerrain_TreeDepth(int depth);


    void SetMaterialID(UINT id) { mMaterialID = id; }

    virtual void Update() override;

    void UpdateLOD(const XMFLOAT3& cameraPos);


    const std::vector<TerrainInstanceData>& GetDrawList() const;
    std::shared_ptr<Mesh> GetMesh() const { return mPatchMesh; }
    UINT GetHeightMapID() const { return mTerrainRes ? mTerrainRes->GetHeightMapID() : Engine::INVALID_ID; }
    UINT GetMaterialID() const { return mMaterialID; }
    float GetHeight(XMFLOAT3 worldPos);

private:
    std::weak_ptr<TransformComponent> mTransform;

    std::unique_ptr<TerrainQuadTree> mQuadTree;
    std::shared_ptr<TerrainResource> mTerrainRes;
    UINT mTerrainResID = Engine::INVALID_ID;
    UINT mMaterialID = Engine::INVALID_ID;

    std::shared_ptr<Mesh> mPatchMesh;

    float mWidth = 1000.0f;
    float mDepth = 1000.0f;
    float mMaxHeight = 100.0f;

    int mTreeDepth = 5;
};