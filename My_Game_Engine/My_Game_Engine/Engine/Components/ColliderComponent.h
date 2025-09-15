#pragma once
#include "../Core/Component.h"

class ColliderComponent : public Component 
{
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