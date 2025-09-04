#pragma once
#include "../Core/Component.h"

class TransformComponent : public Component {
public:
    XMFLOAT3 position{ 0,0,0 };
    XMFLOAT3 rotation{ 0,0,0 };
    XMFLOAT3 scale{ 1,1,1 };

    XMMATRIX GetWorldMatrix() const;
};
