#pragma once
#include "../Core/Component.h"

class TransformComponent : public Component 
{
public:
    static constexpr Component_Type Type = Component_Type::Transform;
    Component_Type GetType() const override { return Type; }

public:
    TransformComponent();

    XMMATRIX GetWorldMatrix() const;

public:
    XMFLOAT4X4 matrix;
    XMFLOAT3 position{ 0,0,0 };
    XMFLOAT3 rotation{ 0,0,0 };
    XMFLOAT3 scale{ 1,1,1 };
};
