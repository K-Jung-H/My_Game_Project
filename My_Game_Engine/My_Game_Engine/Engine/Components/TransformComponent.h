#pragma once
#include "Core/Component.h"

class TransformComponent : public Component
{
public:
    virtual rapidjson::Value ToJSON(rapidjson::Document::AllocatorType& alloc) const;
    virtual void FromJSON(const rapidjson::Value& val);

public:
    static constexpr Component_Type Type = Component_Type::Transform;
    Component_Type GetType() const override { return Type; }

public:
    TransformComponent();

    bool UpdateTransform(const XMFLOAT4X4* parentWorld, bool parentWorldDirty);

    bool GetUpdateFlag() const { return mUpdateFlag; }
    void SetUpdateFlag() { mUpdateFlag = true; }

    const XMFLOAT3& GetPosition() const { return mPosition; }
    const XMFLOAT4& GetRotationQuaternion() const { return mRotation; }
    const XMFLOAT3 GetRotationEuler() const { return mEulerCache; }
    const XMFLOAT3& GetScale() const { return mScale; }


    void AddPosition(const XMFLOAT3& dp);
    void AddRotate(const XMFLOAT4& deltaQuat);
    void AddScale(const XMFLOAT3& ds);

    void SetPosition(const XMFLOAT3& pos) { mPosition = pos; mUpdateFlag = true; }
    void SetRotationQuaternion(const XMFLOAT4& rot);
    void SetRotation_PYR(float pitch, float yaw, float roll);
    void SetRotationEuler(const XMFLOAT3& eulerDeg);

    void SetScale(const XMFLOAT3& scl) { mScale = scl;  mUpdateFlag = true; }

    void SetPose(const XMFLOAT3& pos, const XMFLOAT4& rot) { mPosition = pos; mRotation = rot; mUpdateFlag = true; }

    void SetFromMatrix(const XMFLOAT4X4& mat);
    const XMFLOAT4X4& GetWorldMatrix() const { return mWorld; }


    void SetCbOffset(UINT frameIndex, UINT offset) { mCbOffsets[frameIndex] = offset; }
    UINT GetCbOffset(UINT frameIndex) const { return mCbOffsets[frameIndex]; }

    void ApplyMovePhysics(const XMFLOAT3& linearDelta, const XMFLOAT3& angVel, float dt);


private:
    bool mUpdateFlag = false;

    XMFLOAT3 mPosition{ 0.0f, 0.0f, 0.0f };
    XMFLOAT4 mRotation{ 0.0f, 0.0f, 0.0f, 1.0f };
    XMFLOAT3 mScale{ 1.0f, 1.0f, 1.0f };

    XMFLOAT4X4 mLocal{};
    XMFLOAT4X4 mWorld{};

    std::array<UINT, Engine::Frame_Render_Buffer_Count> mCbOffsets;

    // For Inspector
    XMFLOAT3 mEulerCache{ 0,0,0 };
};
