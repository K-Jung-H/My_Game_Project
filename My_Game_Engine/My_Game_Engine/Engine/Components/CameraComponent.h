#pragma once
#include "Core/Component.h"
#include <DirectXMath.h>

struct RendererContext;
class TransformComponent;

struct CameraCB
{
    XMFLOAT4X4 View;
    XMFLOAT4X4 Proj;
    XMFLOAT4X4 InvView;
    XMFLOAT4X4 InvProj;
    XMFLOAT4X4 InvViewProj;
    XMFLOAT3   CameraPos;
    float      Padding1;
    float      NearZ;
    float      FarZ;
    XMFLOAT2   Padding2;

    UINT ClusterCountX;
    UINT ClusterCountY;
    UINT ClusterCountZ;
    float Padding3;
};

struct ClusterBound
{
    XMFLOAT3 minPoint;
    float pad0;
    XMFLOAT3 maxPoint;
    float pad1;
};

constexpr UINT CLUSTER_X = 16;
constexpr UINT CLUSTER_Y = 9;
constexpr UINT CLUSTER_Z = 24;
constexpr UINT TOTAL_CLUSTER_COUNT = CLUSTER_X * CLUSTER_Y * CLUSTER_Z;

class CameraComponent : public Component 
{
public:
    virtual rapidjson::Value ToJSON(rapidjson::Document::AllocatorType& alloc) const;
    virtual void FromJSON(const rapidjson::Value& val);

public:
    static constexpr Component_Type Type = Component_Type::Camera;
    Component_Type GetType() const override { return Type; }

public:
    CameraComponent();

    void SetTransform(std::weak_ptr<TransformComponent> tf) { mTransform = tf; }
    std::shared_ptr<TransformComponent> GetTransform() { return mTransform.lock(); }

    void CreateCBV(const RendererContext& ctx);
    void UpdateCBV();
    void Graphics_Bind(ComPtr<ID3D12GraphicsCommandList> cmdList, UINT rootParamIndex);
    void Compute_Bind(ComPtr<ID3D12GraphicsCommandList> cmdList, UINT rootParamIndex);

    virtual void Update();

    void SetPosition(const XMFLOAT3& pos);
    const XMFLOAT3& GetPosition();

    void SetRotation(float yaw, float pitch, float roll);
    void AddRotation(float yaw, float pitch, float roll);
    void SetDirection(const XMFLOAT3& dir);
    void SetTargetPosition(const XMFLOAT3& target);
    void SetTargetUse(bool useTarget) { mUseFocusTarget = useTarget; }

    const XMFLOAT3& GetTargetPosition() { return mTarget; }
    bool GetTargetUse() { return mUseFocusTarget; }

    void SetViewport(XMUINT2 LeftTop, XMUINT2 RightBottom);
    void SetScissorRect(XMUINT2 LeftTop, XMUINT2 RightBottom);
    void SetViewportsAndScissorRects(ComPtr<ID3D12GraphicsCommandList> cmdList);

    void UpdateFrustum();
    const BoundingFrustum& GetFrustumWS() const { return mFrustumWS; }

    void SetFovY(float fovY) { mFovY = fovY; mProjDirty = true; }
    void SetNearZ(float nearZ) { mNearZ = nearZ; mProjDirty = true; }
    void SetFarZ(float farZ) { mFarZ = farZ; mProjDirty = true; }

    float GetFovY() { return mFovY; }
    float GetNearZ() { return mNearZ; }
    float GetFarZ() { return mFarZ; }
	float GetAspectRatio() const { return mViewport.Width / mViewport.Height; }

    XMMATRIX GetViewMatrix() const { return XMLoadFloat4x4(&mf4x4View); }
    XMMATRIX GetProjectionMatrix() const { return XMLoadFloat4x4(&mf4x4Projection); }

    bool IsViewMatrixUpdatedThisFrame() const { return mFrameViewMatrixUpdated; }

private:
    float mPitch = 0.0f; 
    float mYaw = 0.0f;

private:
    std::weak_ptr<TransformComponent> mTransform;

    bool mUseFocusTarget = false;
    XMFLOAT3 mTarget{ 0.0f, 0.0f, 0.0f };

    D3D12_VIEWPORT mViewport{};
    D3D12_RECT     mScissorRect{};

    XMFLOAT4X4 mf4x4View{};
    XMFLOAT4X4 mf4x4Projection{};

    float mFovY;
    float mNearZ;
    float mFarZ;

    bool mViewDirty = true;
    bool mProjDirty = true;
	bool mFrameViewMatrixUpdated = false;

    ComPtr<ID3D12Resource> mCameraCB;
    CameraCB* mMappedCB = nullptr;

    BoundingFrustum mFrustumWS;
};
