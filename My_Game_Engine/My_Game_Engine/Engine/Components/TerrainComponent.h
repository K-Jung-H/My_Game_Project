#pragma once
#include "Core/Component.h"
#include "Terrain/TerrainQuadTree.h"
#include "Resource/TerrainResource.h"


class Mesh;
class Texture;
class Material;
class TransformComponent;
class CameraComponent;

class TerrainComponent : public Component
{
public:
    //virtual rapidjson::Value ToJSON(rapidjson::Document::AllocatorType& alloc) const;
    //virtual void FromJSON(const rapidjson::Value& val);

    static constexpr Component_Type Type = Component_Type::Terrain;
    virtual Component_Type GetType() const override { return Type; }

public:
    TerrainComponent();
    virtual ~TerrainComponent();

    virtual void Update() override;

public:
    void SetTransform(std::weak_ptr<TransformComponent> tf);
    std::shared_ptr<TransformComponent> GetTransform() { return mTransform.lock(); }

public:
    void SetTerrain(UINT TerrainResourceID);
    void SetMaterialID(UINT id) { mMaterialID = id; }

    void SetTerrain_Size(float width, float depth, float maxHeight);
    void SetTerrain_Width(float width);
    void SetTerrain_Depth(float depth);
    void SetTerrain_MaxHeight(float maxHeight);
    void SetTerrain_TreeDepth(int depth);

public:
    float GetWidth() const { return mWidth; }
    float GetDepth() const { return mDepth; }
    float GetMaxHeight() const { return mMaxHeight; }
    int GetTreeDepth() const { return mTreeDepth; }

    UINT GetMaterialID() const { return mMaterialID; }
    std::shared_ptr<TerrainResource> GetTerrainResource() const { return mTerrainRes; }
    std::shared_ptr<Mesh> GetMesh() const { return mPatchMesh; }
    UINT GetHeightMapTextureResourceID() const { return mTerrainRes ? mTerrainRes->GetHeightMapTextureResourceID() : Engine::INVALID_ID; }

public:
    void UpdateLOD(CameraComponent* camera);
    float GetHeight(XMFLOAT3 worldPos);
    const std::vector<TerrainInstanceData>& GetDrawList() const;

private:
    std::weak_ptr<TransformComponent> mTransform;

    std::unique_ptr<TerrainQuadTree> mQuadTree;
    std::shared_ptr<TerrainResource> mTerrainRes;
    std::shared_ptr<Mesh> mPatchMesh;

    UINT mTerrainResID = Engine::INVALID_ID;
    UINT mMaterialID = Engine::INVALID_ID;

    float mWidth = 1000.0f;
    float mDepth = 1000.0f;
    float mMaxHeight = 100.0f;
    int mTreeDepth = 5;
};