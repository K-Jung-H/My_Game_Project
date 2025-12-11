#pragma once
#include "Core/Component.h"
#include "Terrain/TerrainQuadTree.h"
#include "Terrain/TerrainHeightField.h"

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

    void Initialize(std::string heightMapPath, float width, float depth, float maxHeight);

    void SetTransform(std::weak_ptr<TransformComponent> tf) { mTransform = tf; }
    std::shared_ptr<TransformComponent> GetTransform() { return mTransform.lock(); }

    virtual void Update() override;
    void UpdateLOD(const XMFLOAT3& cameraPos);

    const std::vector<TerrainInstanceData>& GetDrawList() const;
    std::shared_ptr<Mesh> GetMesh() const { return mPatchMesh; }
    std::shared_ptr<Texture> GetHeightMap() const { return mHeightMap; }
    std::shared_ptr<Material> GetMaterial() const { return mMaterial; }

    float GetHeight(XMFLOAT3 worldPos);

private:
    std::weak_ptr<TransformComponent> mTransform;

    std::shared_ptr<Texture> mHeightMap;
    std::shared_ptr<Mesh> mPatchMesh;
    std::shared_ptr<Material> mMaterial;

    std::unique_ptr<TerrainQuadTree> mQuadTree;
    std::unique_ptr<TerrainHeightField> mHeightField;

    float mWidth = 0.0f;
    float mDepth = 0.0f;
    float mMaxHeight = 0.0f;
};