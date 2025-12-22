#pragma once
#include "Core/Component.h"


class RigidbodyComponent : public SynchronizedComponent
{
public:
    virtual rapidjson::Value ToJSON(rapidjson::Document::AllocatorType& alloc) const;
    virtual void FromJSON(const rapidjson::Value& val);

public:
    static constexpr Component_Type Type = Component_Type::Rigidbody;
    Component_Type GetType() const override { return Type; }

public:
    RigidbodyComponent() = default;

    void Update(float dt);

    const XMFLOAT3& GetVelocity() const { return mVelocity; }
    const XMFLOAT3& GetAcceleration() const { return mAcceleration; }
    const XMFLOAT3& GetAngularVelocity() const { return mAngularVelocity; }
    const bool GetUseGravity() { return mUseGravity; }
    const XMFLOAT3& GetGravity() const { return mGravity; }

    void SetVelocity(const XMFLOAT3& v) { mVelocity = v; }
    void SetAcceleration(const XMFLOAT3& a) { mAcceleration = a; }
    void SetAngularVelocity(const XMFLOAT3& av) { mAngularVelocity = av; }
    void SetGravity(const XMFLOAT3& g) { mGravity = g; }

    void AddVelocity(const XMFLOAT3& v);
    void AddAcceleration(const XMFLOAT3& a);
    void AddAngularVelocity(const XMFLOAT3& av);
    void AddForce(const XMFLOAT3& f);
    void AddTorque(const XMFLOAT3& t);

    void SetKinematic(bool v) { mIsKinematic = v; }
    void SetUseGravity(bool v) { mUseGravity = v; }

    bool IsKinematic() const { return mIsKinematic; }

    void SetMass(float m) { mMass = m; }
    float GetMass() const { return mMass; }

    void SetLinearDamping(float d) { mLinearDamping = d; }
    void SetAngularDamping(float d) { mAngularDamping = d; }

    float GetLinearDamping() const { return mLinearDamping; }
    float GetAngularDamping() const { return mAngularDamping; }

private:
    XMFLOAT3 mVelocity { 0,0,0 };
    XMFLOAT3 mAcceleration { 0,0,0 };
    XMFLOAT3 mAngularVelocity { 0,0,0 };

    XMFLOAT3 mForceAccum { 0,0,0 };
    XMFLOAT3 mTorqueAccum { 0,0,0 };

    XMFLOAT3 mGravity{ 0, -9.8f, 0 };

    float mMass = 1.0f;
    float mLinearDamping = 0.98f;
    float mAngularDamping = 0.98f;

    bool mIsKinematic = false;
    bool mUseGravity = true;
}; 
