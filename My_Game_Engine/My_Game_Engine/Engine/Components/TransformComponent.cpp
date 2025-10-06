#include "TransformComponent.h"
#include "DXMathUtils.h" 

TransformComponent::TransformComponent()
{
    XMStoreFloat4x4(&mWorld, XMMatrixIdentity());
    mPosition = { 0,0,0 };
    mRotation = { 0,0,0,1 };
    mScale = { 1,1,1 };
    mCbOffsets.fill(UINT_MAX);


    XMStoreFloat4x4(&mWorld, XMMatrixIdentity());
    XMStoreFloat4x4(&mLocal, XMMatrixIdentity());

    
}

void TransformComponent::SetRotation_PYR(float pitch, float yaw, float roll)
{
    XMFLOAT4 quat = Matrix4x4::QuaternionFromEuler(pitch, yaw, roll);
    SetRotationQuaternion(quat); 
}

XMFLOAT3 TransformComponent::GetRotationEuler() const
{
    XMVECTOR q = XMLoadFloat4(&mRotation);
    XMMATRIX rotMat = XMMatrixRotationQuaternion(q);

    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, rotMat);

    float pitch, yaw, roll;

    float sy = -m._32;
    sy = std::clamp(sy, -1.0f, 1.0f);
    pitch = asinf(sy);
    yaw = atan2f(m._31, m._33);
    roll = atan2f(m._12, m._22);

    return XMFLOAT3(
        XMConvertToDegrees(pitch),
        XMConvertToDegrees(yaw),
        XMConvertToDegrees(roll)
    );
}

void TransformComponent::SetRotationEuler(const XMFLOAT3& eulerDeg)
{
    float pitch = XMConvertToRadians(eulerDeg.x);
    float yaw = XMConvertToRadians(eulerDeg.y);
    float roll = XMConvertToRadians(eulerDeg.z);
    mRotation = Matrix4x4::QuaternionFromEuler(pitch, yaw, roll);
    mUpdateFlag = true;
}

void TransformComponent::AddPosition(const XMFLOAT3& dp)
{
    mPosition.x += dp.x;
    mPosition.y += dp.y;
    mPosition.z += dp.z;
    mUpdateFlag = true;
}

void TransformComponent::AddRotate(const XMFLOAT4& deltaQuat)
{
    XMVECTOR dq = XMLoadFloat4(&deltaQuat);
    dq = XMQuaternionNormalize(dq);

    XMVECTOR q = XMLoadFloat4(&mRotation);
    q = XMQuaternionMultiply(dq, q);
    q = XMQuaternionNormalize(q);

    XMStoreFloat4(&mRotation, q);
    mUpdateFlag = true;
}

void TransformComponent::AddScale(const XMFLOAT3& ds)
{
    mScale.x += ds.x;
    mScale.y += ds.y;
    mScale.z += ds.z;
    mUpdateFlag = true;
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

        XMStoreFloat4x4(&mLocal, M);

        mUpdateFlag = true;
    }
    else
    {
        mScale = { 1,1,1 };
        mRotation = { 0,0,0,1 };
        mPosition = { 0,0,0 };

        XMStoreFloat4x4(&mLocal, XMMatrixIdentity());
        XMStoreFloat4x4(&mWorld, XMMatrixIdentity());

        mUpdateFlag = true;
    }
}

bool TransformComponent::UpdateTransform(const XMFLOAT4X4* parentWorld, bool parentWorldDirty)
{
    bool localDirty = mUpdateFlag;

    if (localDirty) 
    {
        XMMATRIX S = XMMatrixScaling(mScale.x, mScale.y, mScale.z);
        XMMATRIX R = XMMatrixRotationQuaternion(XMLoadFloat4(&mRotation));
        XMMATRIX T = XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);

        XMStoreFloat4x4(&mLocal, S * R * T);
        mUpdateFlag = false;
    }

    bool worldDirty = localDirty || parentWorldDirty;

    if (worldDirty)
    {
        XMMATRIX L = XMLoadFloat4x4(&mLocal);

        XMMATRIX P = XMLoadFloat4x4(parentWorld);
        XMStoreFloat4x4(&mWorld, L * P);

    }

    return worldDirty;
}

void TransformComponent::ApplyMovePhysics(const XMFLOAT3& linearDelta, const XMFLOAT3& angVel, float dt)
{
    AddPosition(linearDelta);

    if (angVel.x != 0 || angVel.y != 0 || angVel.z != 0)
    {
        XMFLOAT4 deltaQuat = Matrix4x4::QuaternionFromEulerRad(angVel.x * dt, angVel.y * dt, angVel.z * dt);
        AddRotate(deltaQuat);
    }
}