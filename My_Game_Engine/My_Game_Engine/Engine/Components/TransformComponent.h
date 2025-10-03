#pragma once
#include "Core/Component.h"

class TransformComponent : public Component
{
public:
    static constexpr Component_Type Type = Component_Type::Transform;
    Component_Type GetType() const override { return Type; }

public:
    TransformComponent();

    bool UpdateTransform(const XMFLOAT4X4* parentWorld, bool parentWorldDirty);

    bool GetUpdateFlag() const { return mUpdateFlag; }
    void SetUpdateFlag() { mUpdateFlag = true; }

    const XMFLOAT3& GetPosition() const { return mPosition; }
    const XMFLOAT4& GetRotation() const { return mRotation; }
    const XMFLOAT3& GetScale() const { return mScale; }

    void SetPosition(const XMFLOAT3& pos) { mPosition = pos; mUpdateFlag = true; }
    void SetRotation(const XMFLOAT4& rot) { mRotation = rot; mUpdateFlag = true; }
    void SetRotation(float pitch, float yaw, float roll);
    void SetScale(const XMFLOAT3& scl) { mScale = scl;  mUpdateFlag = true; }

    void SetFromMatrix(const XMFLOAT4X4& mat);
    const XMFLOAT4X4& GetWorldMatrix() const { return mWorld; }


    void SetCbOffset(UINT frameIndex, UINT offset) { mCbOffsets[frameIndex] = offset; }
    UINT GetCbOffset(UINT frameIndex) const { return mCbOffsets[frameIndex]; }

private:
    bool mUpdateFlag = false;

    XMFLOAT3 mPosition{ 0.0f, 0.0f, 0.0f };
    XMFLOAT4 mRotation{ 0.0f, 0.0f, 0.0f, 1.0f };
    XMFLOAT3 mScale{ 1.0f, 1.0f, 1.0f };

    XMFLOAT4X4 mLocal{};
    XMFLOAT4X4 mWorld{};

    std::array<UINT, Engine::Frame_Render_Buffer_Count> mCbOffsets;

    //===========================================================
public:

    void UpdateMotion(float dt);

    const XMFLOAT3& GetVelocity() const { return mVelocity; }
    const XMFLOAT3& GetAcceleration() const { return mAcceleration; }
    const XMFLOAT3& GetAngularVelocity() const { return mAngularVelocity; }

    void SetVelocity(const XMFLOAT3& v) { mVelocity = v; mUpdateFlag = true; }
    void SetAcceleration(const XMFLOAT3& a) { mAcceleration = a; mUpdateFlag = true; }
    void SetAngularVelocity(const XMFLOAT3& av) { mAngularVelocity = av; mUpdateFlag = true; }

    void AddVelocity(const XMFLOAT3& v);
    void AddAcceleration(const XMFLOAT3& a);
    void AddAngularVelocity(const XMFLOAT3& av);



private:
    XMFLOAT3 mVelocity{ 0,0,0 };
    XMFLOAT3 mAcceleration{ 0,0,0 };
    XMFLOAT3 mAngularVelocity{ 0,0,0 };

    float mLinearDamping = 0.98f;
    float mAngularDamping = 0.98f;
};
