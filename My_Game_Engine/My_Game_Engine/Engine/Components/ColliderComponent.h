#pragma once
#include "../Core/Component.h"

// Simple sphere collider (for now)
class ColliderComponent : public Component {
public:
    ColliderComponent(float radius = 1.0f);

    float GetRadius() const { return mRadius; }
    void SetRadius(float r) { mRadius = r; }

private:
    float mRadius;
};