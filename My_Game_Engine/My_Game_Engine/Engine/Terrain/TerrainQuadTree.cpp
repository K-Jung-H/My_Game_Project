#include "TerrainQuadTree.h"
#include "DXMathUtils.h"

TerrainQuadTree::TerrainQuadTree()
    : mWidth(0), mDepth(0), mMaxHeight(0), mMaxDepth(0)
{
}

TerrainQuadTree::~TerrainQuadTree()
{
}

void TerrainQuadTree::Initialize(float mapWidth, float mapDepth, float maxHeight, int maxDepth)
{
    mWidth = mapWidth;
    mDepth = mapDepth;
    mMaxHeight = maxHeight;
    mMaxDepth = maxDepth;

    float maxSize = std::max(mWidth, mDepth);
    float centerX = mWidth * 0.5f;
    float centerZ = mDepth * 0.5f;

    mRoot = std::make_unique<TerrainNode>(centerX, centerZ, maxSize, 0);

    mRoot->LocalAABB.Center = XMFLOAT3(centerX, mMaxHeight * 0.5f, centerZ);
    mRoot->LocalAABB.Extents = XMFLOAT3(maxSize * 0.5f, mMaxHeight * 0.5f, maxSize * 0.5f);

    BuildRecursive(mRoot.get());
}

void TerrainQuadTree::BuildRecursive(TerrainNode* node)
{
    if (node->Depth >= mMaxDepth)
        return;

    float quarterSize = node->Size * 0.25f;
    float childSize = node->Size * 0.5f;
    int nextDepth = node->Depth + 1;
    float childY = mMaxHeight * 0.5f;
    float childExtentsY = mMaxHeight * 0.5f;

    node->Children[0] = std::make_unique<TerrainNode>(node->X - quarterSize, node->Z + quarterSize, childSize, nextDepth);
    node->Children[1] = std::make_unique<TerrainNode>(node->X + quarterSize, node->Z + quarterSize, childSize, nextDepth);
    node->Children[2] = std::make_unique<TerrainNode>(node->X - quarterSize, node->Z - quarterSize, childSize, nextDepth);
    node->Children[3] = std::make_unique<TerrainNode>(node->X + quarterSize, node->Z - quarterSize, childSize, nextDepth);

    for (auto& child : node->Children)
    {
        child->LocalAABB.Center = XMFLOAT3(child->X, childY, child->Z);
        child->LocalAABB.Extents = XMFLOAT3(childSize * 0.5f, childExtentsY, childSize * 0.5f);
        BuildRecursive(child.get());
    }
}

void TerrainQuadTree::Update(XMFLOAT3 cameraWorldPos, const XMMATRIX& terrainWorldMatrix)
{
    mDrawList.clear();

    XMVECTOR det = XMMatrixDeterminant(terrainWorldMatrix);
    XMMATRIX invWorld = XMMatrixInverse(&det, terrainWorldMatrix);

    XMVECTOR camPosVec = XMLoadFloat3(&cameraWorldPos);
    XMVECTOR camLocalPosVec = XMVector3TransformCoord(camPosVec, invWorld);

    XMFLOAT3 camLocalPos;
    XMStoreFloat3(&camLocalPos, camLocalPosVec);

    CullAndSelectLOD(mRoot.get(), camLocalPos);
}

void TerrainQuadTree::CullAndSelectLOD(TerrainNode* node, XMFLOAT3 cameraLocalPos)
{
    float distSq = Vector3::DistanceSquared(cameraLocalPos, XMFLOAT3(node->X, 0.0f, node->Z));

    float lodThreshold = node->Size * mLODDistanceRatio;
    bool shouldSplit = (distSq < lodThreshold * lodThreshold) && (!node->IsLeaf());

    if (shouldSplit)
    {
        for (auto& child : node->Children)
        {
            CullAndSelectLOD(child.get(), cameraLocalPos);
        }
    }
    else
    {
        TerrainInstanceData instance;

        float halfSize = node->Size * 0.5f;
        instance.LocalOffset = XMFLOAT3(node->X - halfSize, 0.0f, node->Z - halfSize);
        instance.Scale = node->Size;

        instance.UVOffset.x = (instance.LocalOffset.x) / mWidth;
        instance.UVOffset.y = (instance.LocalOffset.z) / mDepth;
        instance.UVScale.x = node->Size / mWidth;
        instance.UVScale.y = node->Size / mDepth;

        mDrawList.push_back(instance);
    }
}