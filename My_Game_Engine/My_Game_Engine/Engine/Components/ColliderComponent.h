#pragma once
#include "Core/Component.h"

enum class Collider_Type
{
    Sphere,
    Box,
    Capsule,
    Mesh,
    Terrain,
    Etc
};

class ColliderComponent : public DataComponent
{
public:
    virtual rapidjson::Value ToJSON(rapidjson::Document::AllocatorType& alloc) const override;
    virtual void FromJSON(const rapidjson::Value& val) override;

public:
    static constexpr Component_Type Type = Component_Type::Collider;
    Component_Type GetType() const override { return Type; }

public:
    ColliderComponent();
    ~ColliderComponent() = default;

    Collider_Type GetColliderType() const { return mColliderType; }
    void SetColliderType(Collider_Type type) { mColliderType = type; }

    const XMFLOAT3& GetCenter() const { return mCenter; }
    void SetCenter(const XMFLOAT3& center) { mCenter = center; }
    void SetCenter(float x, float y, float z) { mCenter = XMFLOAT3(x, y, z); }

    float GetRadius() const { return mRadius; }
    void SetRadius(float radius) { mRadius = radius; }

    const XMFLOAT3& GetSize() const { return mSize; }
    void SetSize(const XMFLOAT3& size) { mSize = size; }
    void SetSize(float x, float y, float z) { mSize = XMFLOAT3(x, y, z); }

    float GetHeight() const { return mHeight; }
    void SetHeight(float height) { mHeight = height; }

private:
    Collider_Type mColliderType = Collider_Type::Sphere;

    XMFLOAT3 mCenter = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 mSize = { 1.0f, 1.0f, 1.0f };
    float mRadius = 0.5f;
    float mHeight = 1.0f;
};