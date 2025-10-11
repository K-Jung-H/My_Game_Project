#include "RigidbodyComponent.h"


rapidjson::Value RigidbodyComponent::ToJSON(rapidjson::Document::AllocatorType& alloc) const
{
    Value v(kObjectType);
    v.AddMember("type", "RigidbodyComponent", alloc);
    v.AddMember("isKinematic", mIsKinematic, alloc);
    v.AddMember("useGravity", mUseGravity, alloc);
    v.AddMember("mass", mMass, alloc);
    v.AddMember("linearDamping", mLinearDamping, alloc);
    v.AddMember("angularDamping", mAngularDamping, alloc);

    Value grav(kArrayType);
    grav.PushBack(mGravity.x, alloc).PushBack(mGravity.y, alloc).PushBack(mGravity.z, alloc);
    v.AddMember("gravity", grav, alloc);
    return v;
}

void RigidbodyComponent::FromJSON(const rapidjson::Value& val)
{
    mIsKinematic = val["isKinematic"].GetBool();
    mUseGravity = val["useGravity"].GetBool();
    mMass = val["mass"].GetFloat();
    mLinearDamping = val["linearDamping"].GetFloat();
    mAngularDamping = val["angularDamping"].GetFloat();

    const auto& g = val["gravity"].GetArray();
    mGravity = { (float)g[0].GetDouble(), (float)g[1].GetDouble(), (float)g[2].GetDouble() };
}


void RigidbodyComponent::AddVelocity(const XMFLOAT3& v)
{
    mVelocity.x += v.x;
    mVelocity.y += v.y;
    mVelocity.z += v.z;
}

void RigidbodyComponent::AddAcceleration(const XMFLOAT3& a)
{
    mAcceleration.x += a.x;
    mAcceleration.y += a.y;
    mAcceleration.z += a.z;
}

void RigidbodyComponent::AddAngularVelocity(const XMFLOAT3& av)
{
    mAngularVelocity.x += av.x;
    mAngularVelocity.y += av.y;
    mAngularVelocity.z += av.z;
}

void RigidbodyComponent::AddForce(const XMFLOAT3& f)
{
    mForceAccum.x += f.x;
    mForceAccum.y += f.y;
    mForceAccum.z += f.z;
}

void RigidbodyComponent::AddTorque(const XMFLOAT3& t)
{
    mTorqueAccum.x += t.x;
    mTorqueAccum.y += t.y;
    mTorqueAccum.z += t.z;
}


void RigidbodyComponent::Update(float dt)
{
    if (mIsKinematic) return; 

    XMFLOAT3 accel = mAcceleration;
    accel.x += mForceAccum.x / mMass;
    accel.y += mForceAccum.y / mMass;
    accel.z += mForceAccum.z / mMass;

    if (mUseGravity) 
    {
        accel.x += mGravity.x;
        accel.y += mGravity.y;
        accel.z += mGravity.z;
    }


    mVelocity.x += accel.x * dt;
    mVelocity.y += accel.y * dt;
    mVelocity.z += accel.z * dt;

    mVelocity.x *= mLinearDamping;
    mVelocity.y *= mLinearDamping;
    mVelocity.z *= mLinearDamping;

    mAngularVelocity.x *= mAngularDamping;
    mAngularVelocity.y *= mAngularDamping;
    mAngularVelocity.z *= mAngularDamping;

    mForceAccum = { 0,0,0 };
    mTorqueAccum = { 0,0,0 };
}
