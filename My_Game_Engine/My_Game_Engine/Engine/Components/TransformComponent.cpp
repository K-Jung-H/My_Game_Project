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

rapidjson::Value TransformComponent::ToJSON(rapidjson::Document::AllocatorType& alloc) const
{
    Value v(kObjectType);
    v.AddMember("type", "TransformComponent", alloc);

    Value pos(kArrayType); pos.PushBack(mPosition.x, alloc).PushBack(mPosition.y, alloc).PushBack(mPosition.z, alloc);
    Value rot(kArrayType); rot.PushBack(mRotation.x, alloc).PushBack(mRotation.y, alloc).PushBack(mRotation.z, alloc).PushBack(mRotation.w, alloc);
    Value scl(kArrayType); scl.PushBack(mScale.x, alloc).PushBack(mScale.y, alloc).PushBack(mScale.z, alloc);

    v.AddMember("position", pos, alloc);
    v.AddMember("rotation", rot, alloc);
    v.AddMember("scale", scl, alloc);
    return v;
}

void TransformComponent::FromJSON(const rapidjson::Value& val)
{
    const auto& pos = val["position"].GetArray();
    const auto& rot = val["rotation"].GetArray();
    const auto& scl = val["scale"].GetArray();

    mPosition = { (float)pos[0].GetDouble(), (float)pos[1].GetDouble(), (float)pos[2].GetDouble() };
    mRotation = { (float)rot[0].GetDouble(), (float)rot[1].GetDouble(), (float)rot[2].GetDouble(), (float)rot[3].GetDouble() };
    mScale = { (float)scl[0].GetDouble(), (float)scl[1].GetDouble(), (float)scl[2].GetDouble() };
}

void TransformComponent::SetRotation_PYR(float pitch, float yaw, float roll)
{
    SetRotationEuler(XMFLOAT3(pitch, yaw, roll));
}

void TransformComponent::SetRotationEuler(const XMFLOAT3& eulerDeg)
{
    mEulerCache = eulerDeg;

    XMVECTOR q = XMQuaternionRotationRollPitchYaw(
        XMConvertToRadians(eulerDeg.x),
        XMConvertToRadians(eulerDeg.y),
        XMConvertToRadians(eulerDeg.z)
    );

    XMStoreFloat4(&mRotation, q);

    mUpdateFlag = true;
}

void TransformComponent::SetRotationQuaternion(const XMFLOAT4& quat)
{
    mRotation = quat;

    XMVECTOR q = XMLoadFloat4(&quat);
    float rollR, pitchR, yawR;
    XMQuaternionToRollPitchYaw(q, &rollR, &pitchR, &yawR);

    mEulerCache = XMFLOAT3(
        XMConvertToDegrees(pitchR),
        XMConvertToDegrees(yawR),
        XMConvertToDegrees(rollR)
    );

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