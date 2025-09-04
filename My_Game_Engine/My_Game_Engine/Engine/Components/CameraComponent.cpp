#include "pch.h"
#include "CameraComponent.h"

CameraComponent::CameraComponent()
    : mPosition(0.0f, 0.0f, -5.0f),
    mTarget(0.0f, 0.0f, 0.0f),
    mUp(0.0f, 1.0f, 0.0f),
    mFovY(XM_PIDIV4),
    mNearZ(0.1f),
    mFarZ(1000.0f) {
}

void CameraComponent::SetPosition(const XMFLOAT3& pos) { mPosition = pos; }
void CameraComponent::SetTarget(const XMFLOAT3& tgt) { mTarget = tgt; }
void CameraComponent::SetUp(const XMFLOAT3& up) { mUp = up; }

XMMATRIX CameraComponent::GetViewMatrix() const {
    return XMMatrixLookAtLH(XMLoadFloat3(&mPosition),
        XMLoadFloat3(&mTarget),
        XMLoadFloat3(&mUp));
}

XMMATRIX CameraComponent::GetProjectionMatrix(float aspectRatio) const {
    return XMMatrixPerspectiveFovLH(mFovY, aspectRatio, mNearZ, mFarZ);
}
