#pragma once
#include "../Core/Component.h"
#include <DirectXMath.h>

struct RendererContext;

struct CameraCB
{
    XMFLOAT4X4 View;
    XMFLOAT4X4 Proj;
    XMFLOAT3   CameraPos;
    float               Padding;
};

class CameraComponent : public Component 
{
public:
    static constexpr Component_Type Type = Component_Type::Camera;
    Component_Type GetType() const override { return Type; }

public:
    CameraComponent();

    void CreateCBV(const RendererContext& ctx);
    void UpdateCBV();
    void Bind(ComPtr<ID3D12GraphicsCommandList> cmdList, UINT rootParamIndex);

    void SetViewport(XMUINT2 LeftTop, XMUINT2 RightBottom);
    void SetScissorRect(XMUINT2 LeftTop, XMUINT2 RightBottom);
    void SetViewportsAndScissorRects(ComPtr<ID3D12GraphicsCommandList> cmdList);
    void SetPosition(const XMFLOAT3& pos);
    void SetTarget(const XMFLOAT3& tgt);
    void SetUp(const XMFLOAT3& up);


    XMMATRIX GetViewMatrix() const { return XMLoadFloat4x4(&mf4x4View); }
    XMMATRIX GetProjectionMatrix() const { return XMLoadFloat4x4(&mf4x4Projection); }

    virtual void Update();


private:
    ComPtr<ID3D12Resource> mCameraCB;
    CameraCB* mMappedCB = nullptr;

    bool mViewDirty = true;
    bool mProjDirty = true;


    D3D12_VIEWPORT					mViewport;
    D3D12_RECT						mScissorRect;

    XMFLOAT4X4						mf4x4View;
    XMFLOAT4X4						mf4x4Projection;

    DirectX::XMFLOAT3 mPosition;
    DirectX::XMFLOAT3 mTarget;
    DirectX::XMFLOAT3 mUp;

    float mFovY;    
    float mNearZ;
    float mFarZ;
};
