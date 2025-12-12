#include "TerrainComponent.h"
#include "Core/Object.h"
#include "Components/TransformComponent.h"
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

void TerrainComponent::UpdateLOD(const DirectX::XMFLOAT3& cameraPos)
{
    if (!mQuadTree) return;

    std::shared_ptr<TransformComponent> tr = mTransform.lock();
    if (!tr) return;

    XMMATRIX worldMat = XMLoadFloat4x4(&tr->GetWorldMatrix());
    mQuadTree->Update(cameraPos, worldMat);
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
    if (!mTerrainRes || !mTerrainRes->GetHeightField()) return 0.0f;
    if (!mTransform.lock()) return 0.0f;

    std::shared_ptr<TransformComponent> tr = mTransform.lock();
    XMMATRIX worldInv = XMMatrixInverse(nullptr, XMLoadFloat4x4(&tr->GetWorldMatrix()));
    XMVECTOR localPosVec = XMVector3TransformCoord(XMLoadFloat3(&worldPos), worldInv);
    XMFLOAT3 localPos;
    XMStoreFloat3(&localPos, localPosVec);

    float u = localPos.x / mWidth;
    float v = localPos.z / mDepth;

    float normalizedHeight = mTerrainRes->GetHeightField()->GetHeight(u, v);

    return normalizedHeight * mMaxHeight;
}