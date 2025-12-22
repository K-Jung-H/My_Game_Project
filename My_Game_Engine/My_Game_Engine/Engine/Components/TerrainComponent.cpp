#include "TerrainComponent.h"
#include "Core/Object.h"
#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h"
#include "GameEngine.h"
#include "Resource/ResourceSystem.h"

TerrainComponent::TerrainComponent()
{
    mQuadTree = std::make_unique<TerrainQuadTree>();
    mPatchMesh = std::make_shared<TerrainPatchMesh>();

    mTreeDepth = 5;
}

TerrainComponent::~TerrainComponent()
{
}

void TerrainComponent::SetTransform(std::weak_ptr<TransformComponent> tf)
{
    mTransform = tf;
}


void TerrainComponent::SetTerrain(UINT TerrainResourceID)
{
    ResourceSystem* rs = GameEngine::Get().GetResourceSystem();

    mTerrainRes = rs->GetById<TerrainResource>(TerrainResourceID);

    if (!mTerrainRes)
        return;

    mTerrainResID = TerrainResourceID;


    if (mQuadTree)
        mQuadTree->Initialize(mWidth, mDepth, mMaxHeight, mTreeDepth);
}

void TerrainComponent::SetTerrain_Width(float width)
{
    mWidth = width;
    if (mQuadTree)   mQuadTree->Initialize(mWidth, mDepth, mMaxHeight, mTreeDepth);
}

void TerrainComponent::SetTerrain_Depth(float depth)
{
    mDepth = depth;
    if (mQuadTree)   mQuadTree->Initialize(mWidth, mDepth, mMaxHeight, mTreeDepth);
}

void TerrainComponent::SetTerrain_MaxHeight(float maxHeight)
{
    mMaxHeight = maxHeight;
    if (mQuadTree)   mQuadTree->Initialize(mWidth, mDepth, mMaxHeight, mTreeDepth);
}

void TerrainComponent::SetTerrain_TreeDepth(int depth)
{
    if (mTreeDepth != depth)
    {
        mTreeDepth = depth;

        if (mQuadTree)
            mQuadTree->Initialize(mWidth, mDepth, mMaxHeight, mTreeDepth);

    }
}

void TerrainComponent::SetTerrain_Size(float width, float depth, float maxHeight)
{
    mWidth = width;
    mDepth = depth;
    mMaxHeight = maxHeight;

    if (mQuadTree)
        mQuadTree->Initialize(mWidth, mDepth, mMaxHeight, mTreeDepth);

}

void TerrainComponent::Update()
{
    if (mTransform.expired())
    {
        std::shared_ptr<TransformComponent> tr = GetOwner()->GetComponent<TransformComponent>();
        if (tr)
        {
            mTransform = tr;
        }
    }
}

void TerrainComponent::UpdateLOD(CameraComponent* camera)
{
    if (!mQuadTree || !camera) return;

    if (mTransform.expired()) return;

    XMMATRIX worldMat = XMLoadFloat4x4(&mTransform.lock()->GetWorldMatrix());

    mQuadTree->Update(camera, worldMat);
}

const std::vector<TerrainInstanceData>& TerrainComponent::GetDrawList() const
{
    if (mQuadTree)
        return mQuadTree->GetDrawList();

    static const std::vector<TerrainInstanceData> empty;
    return empty;
}

float TerrainComponent::GetHeight(XMFLOAT3 worldPos)
{
    if (!mTerrainRes || !mTerrainRes->GetHeightField()) return -FLT_MAX;
    std::shared_ptr<TransformComponent> tr = mTransform.lock();
    if (!tr) return -FLT_MAX;

    const auto& worldMat = tr->GetWorldMatrix();

    XMMATRIX worldInv = XMMatrixInverse(nullptr, XMLoadFloat4x4(&worldMat));
    XMVECTOR localPosVec = XMVector3TransformCoord(XMLoadFloat3(&worldPos), worldInv);
    XMFLOAT3 localPos;
    XMStoreFloat3(&localPos, localPosVec);

    float u = localPos.x / mWidth;
    float v = localPos.z / mDepth;

    if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) return -FLT_MAX;

    float normalizedHeight = mTerrainRes->GetHeightField()->GetHeight(u, v);
    float localHeight = normalizedHeight * mMaxHeight;

    XMFLOAT3 surfaceLocal = { localPos.x, localHeight, localPos.z };
    XMVECTOR surfaceWorldVec = XMVector3TransformCoord(XMLoadFloat3(&surfaceLocal), XMLoadFloat4x4(&worldMat));

    return XMVectorGetY(surfaceWorldVec);
}