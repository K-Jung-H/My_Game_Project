#include "CameraComponent.h"
#include "TransformComponent.h"
#include "GameEngine.h"
#include "DX_Graphics/Renderer.h"

CameraComponent::CameraComponent()
    : mFovY(XM_PIDIV4), mNearZ(0.1f), mFarZ(1000.0f)
{
    mViewport = { 0, 0, SCREEN_WIDTH , SCREEN_HEIGHT, 0.0f, 1.0f };
    mScissorRect = { 0, 0, SCREEN_WIDTH , SCREEN_HEIGHT };

    XMStoreFloat4x4(&mf4x4View, XMMatrixIdentity());
    XMStoreFloat4x4(&mf4x4Projection, XMMatrixIdentity());

    RendererContext rc = GameEngine::Get().Get_UploadContext();
    CreateCBV(rc);
}

void CameraComponent::CreateCBV(const RendererContext& ctx)
{
    UINT bufferSize = (sizeof(CameraCB) + 255) & ~255;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    HRESULT hr = ctx.device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mCameraCB));
 
    hr = mCameraCB->Map(0, nullptr, reinterpret_cast<void**>(&mMappedCB));

    if (FAILED(hr)) 
        return;

    return;
}

void CameraComponent::UpdateCBV()
{
    CameraCB cb;

    XMMATRIX view = GetViewMatrix();
    XMMATRIX proj = GetProjectionMatrix();
    XMMATRIX viewProj = XMMatrixMultiply(view, proj);

    XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProj);

    XMStoreFloat4x4(&cb.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&cb.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&cb.InvViewProj, XMMatrixTranspose(invViewProj));

    if (auto tf = mTransform.lock())
        cb.CameraPos = tf->GetPosition();
    else
        cb.CameraPos = XMFLOAT3(0.0f, 0.0f, 0.0f); 

    cb.NearZ = mNearZ;
    cb.FarZ = mFarZ;
    cb.Padding = 0.0f;

    memcpy(mMappedCB, &cb, sizeof(CameraCB));
}

void CameraComponent::Bind(ComPtr<ID3D12GraphicsCommandList> cmdList, UINT rootParamIndex)
{
    cmdList->SetGraphicsRootConstantBufferView(rootParamIndex, mCameraCB->GetGPUVirtualAddress());
}

void CameraComponent::SetViewport(XMUINT2 LeftTop, XMUINT2 RightBottom)
{
    float xTopLeft = static_cast<float>(LeftTop.x);
    float yTopLeft = static_cast<float>(LeftTop.y);
    float nWidth = static_cast<float>(RightBottom.x - LeftTop.x);
    float nHeight = static_cast<float>(RightBottom.y - LeftTop.y);

    mViewport.TopLeftX = xTopLeft;
    mViewport.TopLeftY = yTopLeft;
    mViewport.Width = nWidth;
    mViewport.Height = nHeight;
    mViewport.MinDepth = 0.0f;
    mViewport.MaxDepth = 1.0f;

    mProjDirty = true;
}

void CameraComponent::SetScissorRect(XMUINT2 LeftTop, XMUINT2 RightBottom)
{
    mScissorRect.left = static_cast<LONG>(LeftTop.x);
    mScissorRect.top = static_cast<LONG>(LeftTop.y);
    mScissorRect.right = static_cast<LONG>(RightBottom.x);
    mScissorRect.bottom = static_cast<LONG>(RightBottom.y);

}

void CameraComponent::SetViewportsAndScissorRects(ComPtr<ID3D12GraphicsCommandList> cmdList)
{
    cmdList->RSSetViewports(1, &mViewport);
    cmdList->RSSetScissorRects(1, &mScissorRect);
}

void CameraComponent::Update()
{
    if (auto tf = mTransform.lock())
    {
        if (mViewDirty || tf->GetUpdateFlag())
        {
            XMVECTOR pos = XMLoadFloat3(&tf->GetPosition());
            XMVECTOR rotQ = XMLoadFloat4(&tf->GetRotationQuaternion());
            XMVECTOR up = XMVectorSet(0, 1, 0, 0);

            XMMATRIX view;
            if (mUseFocusTarget)
            {
                XMVECTOR tgt = XMLoadFloat3(&mTarget);
                XMVECTOR dir = XMVector3Normalize(XMVectorSubtract(tgt, pos));

                XMMATRIX rotMat = XMMatrixLookToLH(XMVectorZero(), dir, up);
                XMVECTOR q = XMQuaternionRotationMatrix(rotMat);
                XMFLOAT4 qOut;
                XMStoreFloat4(&qOut, q);
                tf->SetRotationQuaternion(qOut);


                view = XMMatrixLookAtLH(pos, tgt, up);
            }
            else
            {
                XMMATRIX rotMat = XMMatrixRotationQuaternion(rotQ);
                XMVECTOR look = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rotMat);
                XMVECTOR upV = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMat);

                view = XMMatrixLookToLH(pos, look, upV);
            }

            XMStoreFloat4x4(&mf4x4View, view);
            mViewDirty = false;
        }

        if (mProjDirty)
        {
            float aspect = mViewport.Width / mViewport.Height;
            XMMATRIX proj = XMMatrixPerspectiveFovLH(mFovY, aspect, mNearZ, mFarZ);
            XMStoreFloat4x4(&mf4x4Projection, proj);
            mProjDirty = false;
        }
    }
}

void CameraComponent::SetPosition(const XMFLOAT3& pos)
{
    if (auto tf = mTransform.lock())
        tf->SetPosition(pos);
    else
        throw std::runtime_error("Failed to Find Camera's Transform");

}

const XMFLOAT3& CameraComponent::GetPosition()
{
    if (auto tf = mTransform.lock())
        return tf->GetPosition();
    else
        throw std::runtime_error("Failed to Find Camera's Transform");
}

void CameraComponent::SetRotation(float yaw, float pitch, float roll)
{
    if (auto tf = mTransform.lock())
    {
        XMMATRIX yawMat = XMMatrixRotationY(yaw);
        XMMATRIX pitchMat = XMMatrixRotationX(pitch);
        XMMATRIX rollMat = XMMatrixRotationZ(roll);

        XMMATRIX rotMat = rollMat * pitchMat * yawMat;

        XMVECTOR q = XMQuaternionRotationMatrix(rotMat);
        XMFLOAT4 qOut;
        XMStoreFloat4(&qOut, q);
        tf->SetRotationQuaternion(qOut);

        mViewDirty = true;
    }
}

void CameraComponent::AddRotation(float pitch, float yaw, float roll)
{
    if (auto tf = mTransform.lock())
    {
        mYaw += yaw;
        mPitch += pitch;

        mPitch = std::clamp(mPitch, XMConvertToRadians(-89.9f), XMConvertToRadians(89.9f));

        XMVECTOR q = XMQuaternionRotationRollPitchYaw(mPitch, mYaw, 0.0f);

        XMFLOAT4 qOut;
        XMStoreFloat4(&qOut, q);
        tf->SetRotationQuaternion(qOut);

        mViewDirty = true;
    }
}

void CameraComponent::SetDirection(const XMFLOAT3& dir)
{
    if (auto tf = mTransform.lock())
    {
        XMVECTOR look = XMVector3Normalize(XMLoadFloat3(&dir));
        XMVECTOR up = XMVectorSet(0, 1, 0, 0);

        XMMATRIX rotMat = XMMatrixLookToLH(XMVectorZero(), look, up);
        XMVECTOR q = XMQuaternionRotationMatrix(rotMat);

        XMFLOAT4 qOut;
        XMStoreFloat4(&qOut, q);
        tf->SetRotationQuaternion(qOut);

        mViewDirty = true;
    }
}

void CameraComponent::SetTargetPosition(const XMFLOAT3& target)
{
    mTarget = target;
    mUseFocusTarget = true;
    mViewDirty = true;
}