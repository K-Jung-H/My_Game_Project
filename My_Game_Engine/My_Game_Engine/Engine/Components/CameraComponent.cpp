#include "CameraComponent.h"
#include "GameEngine.h"
#include "DX_Graphics/Renderer.h"

CameraComponent::CameraComponent()
    : mPosition(0.0f, 0.0f, -5.0f), 
    mTarget(0.0f, 0.0f, 0.0f),
    mUp(0.0f, 1.0f, 0.0f),
    mFovY(XM_PIDIV4),
    mNearZ(0.1f),
    mFarZ(1000.0f) 
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

    XMStoreFloat4x4(&cb.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&cb.Proj, XMMatrixTranspose(proj));
    cb.CameraPos = mPosition;
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

void CameraComponent::SetPosition(const XMFLOAT3& pos) { mPosition = pos; mViewDirty = true; }
void CameraComponent::SetTarget(const XMFLOAT3& tgt) { mTarget = tgt;   mViewDirty = true; }
void CameraComponent::SetUp(const XMFLOAT3& up) { mUp = up;        mViewDirty = true; }


void CameraComponent::Update()
{
    if (mViewDirty) 
    {
        auto view = XMMatrixLookAtLH(XMLoadFloat3(&mPosition),
            XMLoadFloat3(&mTarget),
            XMLoadFloat3(&mUp));
            XMStoreFloat4x4(&mf4x4View, view);
        mViewDirty = false;
    }

    if (mProjDirty) 
    {
        float aspectRatio = mViewport.Width / mViewport.Height;
        auto proj = XMMatrixPerspectiveFovLH(mFovY, aspectRatio, mNearZ, mFarZ);
        XMStoreFloat4x4(&mf4x4Projection, proj);
        mProjDirty = false;
    }
}
