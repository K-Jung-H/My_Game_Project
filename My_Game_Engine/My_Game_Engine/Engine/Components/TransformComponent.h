#pragma once
#include "../Core/Component.h"

class TransformComponent : public Component
{
public:
    static constexpr Component_Type Type = Component_Type::Transform;
    Component_Type GetType() const override { return Type; }

public:
    TransformComponent();

    void SetFromMatrix(const XMFLOAT4X4& mat);

    const XMFLOAT3& GetPosition() const { return mPosition; }
    const XMFLOAT4& GetRotation() const { return mRotation; }
    const XMFLOAT3& GetScale() const { return mScale; }

    void SetPosition(const XMFLOAT3& pos) { mPosition = pos; }
    void SetRotation(const XMFLOAT4& rot) { mRotation = rot; }
    void SetScale(const XMFLOAT3& scl) { mScale = scl; }

    const XMFLOAT4X4& GetWorldMatrix() const { return mWorld; }

    void SetCbOffset(UINT frameIndex, UINT offset) { mCbOffsets[frameIndex] = offset; }
    UINT GetCbOffset(UINT frameIndex) const { return mCbOffsets[frameIndex]; }

private:
    XMFLOAT3 mPosition;
    XMFLOAT4 mRotation;
    XMFLOAT3 mScale;
    XMFLOAT4X4 mWorld;

    std::array<UINT, Engine::Frame_Render_Buffer_Count> mCbOffsets;
};
