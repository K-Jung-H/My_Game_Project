#pragma once
#include "../Core/Component.h"
#include <DirectXMath.h>

class CameraComponent : public Component 
{
public:
    static constexpr Component_Type Type = Component_Type::Camera;
    Component_Type GetType() const override { return Type; }

public:
    CameraComponent();

    void SetPosition(const DirectX::XMFLOAT3& pos);
    void SetTarget(const DirectX::XMFLOAT3& tgt);
    void SetUp(const DirectX::XMFLOAT3& up);

    DirectX::XMMATRIX GetViewMatrix() const;
    DirectX::XMMATRIX GetProjectionMatrix(float aspectRatio) const;

private:
    DirectX::XMFLOAT3 mPosition;
    DirectX::XMFLOAT3 mTarget;
    DirectX::XMFLOAT3 mUp;
    float mFovY;    
    float mNearZ;
    float mFarZ;
};
