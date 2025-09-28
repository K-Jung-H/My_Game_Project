#pragma once
#include "../Core/Component.h"

class TransformComponent : public Component
{
public:
    static constexpr Component_Type Type = Component_Type::Transform;
    Component_Type GetType() const override { return Type; }

public:
    TransformComponent();

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

    bool Update(const XMFLOAT4X4* parentWorld, bool parentWorldDirty);

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
};
