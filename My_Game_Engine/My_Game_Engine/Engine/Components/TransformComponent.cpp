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

void TransformComponent::SetRotation(float pitch, float yaw, float roll)
{
    XMFLOAT4 quat = Matrix4x4::QuaternionFromEuler(pitch, yaw, roll);
    SetRotation(quat); 
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


void TransformComponent::AddVelocity(const XMFLOAT3& v) 
{
    mVelocity.x += v.x;
    mVelocity.y += v.y;
    mVelocity.z += v.z;
    mUpdateFlag = true;
}

void TransformComponent::AddAcceleration(const XMFLOAT3& a) 
{
    mAcceleration.x += a.x;
    mAcceleration.y += a.y;
    mAcceleration.z += a.z;
    mUpdateFlag = true;
}

void TransformComponent::AddAngularVelocity(const XMFLOAT3& av) 
{
    mAngularVelocity.x += av.x;
    mAngularVelocity.y += av.y;
    mAngularVelocity.z += av.z;
    mUpdateFlag = true;
}

void TransformComponent::UpdateMotion(float dt)
{
    mVelocity.x += mAcceleration.x * dt;
    mVelocity.y += mAcceleration.y * dt;
    mVelocity.z += mAcceleration.z * dt;

    mPosition.x += mVelocity.x * dt;
    mPosition.y += mVelocity.y * dt;
    mPosition.z += mVelocity.z * dt;

    if (mAngularVelocity.x != 0.0f || mAngularVelocity.y != 0.0f || mAngularVelocity.z != 0.0f)
    {
        XMVECTOR dq = XMQuaternionRotationRollPitchYaw(
            mAngularVelocity.x * dt,
            mAngularVelocity.y * dt,
            mAngularVelocity.z * dt
        );

        XMVECTOR q = XMLoadFloat4(&mRotation);
        q = XMQuaternionMultiply(dq, q);
        q = XMQuaternionNormalize(q);

        XMStoreFloat4(&mRotation, q);
    }

    mVelocity.x *= mLinearDamping;
    mVelocity.y *= mLinearDamping;
    mVelocity.z *= mLinearDamping;

    mAngularVelocity.x *= mAngularDamping;
    mAngularVelocity.y *= mAngularDamping;
    mAngularVelocity.z *= mAngularDamping;

    mUpdateFlag = true;
}

