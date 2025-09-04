#include "pch.h"
#include "TransformComponent.h"


XMMATRIX TransformComponent::GetWorldMatrix() const {
    return XMMatrixScaling(scale.x, scale.y, scale.z) *
        XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z) *
        XMMatrixTranslation(position.x, position.y, position.z);
}
