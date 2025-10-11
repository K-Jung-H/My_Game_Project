#pragma once
#include "../Core/Component.h"

class ColliderComponent : public Component 
{
public:
    virtual rapidjson::Value ToJSON(rapidjson::Document::AllocatorType& alloc) const;
    virtual void FromJSON(const rapidjson::Value& val);

public:
    static constexpr Component_Type Type = Component_Type::Collider;
    Component_Type GetType() const override { return Type; }

public:
    ColliderComponent(float radius = 1.0f);

    float GetRadius() const { return mRadius; }
    void SetRadius(float r) { mRadius = r; }

private:
    float mRadius;
};