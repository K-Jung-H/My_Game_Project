#include "pch.h"
#include "TransformComponent.h"

TransformComponent::TransformComponent()
{
    XMStoreFloat4x4(&mWorld, XMMatrixIdentity());
    mPosition = { 0,0,0 };
    mRotation = { 0,0,0,1 };
    mScale = { 1,1,1 };
}


void TransformComponent::SetFromMatrix(const XMFLOAT4X4& mat)
{
    XMMATRIX M = XMLoadFloat4x4(&mat);

    XMVECTOR scale;
    XMVECTOR rotQuat;
    XMVECTOR trans;
    if (XMMatrixDecompose(&scale, &rotQuat, &trans, M))
    {
        XMStoreFloat3(&mScale, scale);
        XMStoreFloat4(&mRotation, rotQuat);
        XMStoreFloat3(&mPosition, trans);
        XMStoreFloat4x4(&mWorld, M);
    }
    else
    {
        mScale = { 1,1,1 };
        mRotation = { 0,0,0,1 };
        mPosition = { 0,0,0 };
        XMStoreFloat4x4(&mWorld, XMMatrixIdentity());
    }
}