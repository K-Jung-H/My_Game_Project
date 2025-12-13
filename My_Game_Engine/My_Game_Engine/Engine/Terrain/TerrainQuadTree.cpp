#include "TerrainQuadTree.h"
#include "Components/CameraComponent.h"
#include "DXMathUtils.h"

TerrainNode::TerrainNode(float x, float z, float size, int depth, float maxHeight)
    : X(x), Z(z), Size(size), Depth(depth)
{
    float halfSize = size * 0.5f;
    float halfHeight = maxHeight * 0.5f;

    LocalAABB.Center = XMFLOAT3(x + halfSize, halfHeight, z + halfSize);
    LocalAABB.Extents = XMFLOAT3(halfSize, halfHeight, halfSize);
}

TerrainQuadTree::TerrainQuadTree()
{
}

TerrainQuadTree::~TerrainQuadTree()
{
}

void TerrainQuadTree::Initialize(float width, float depth, float maxHeight, int maxDepth)
{
    mMaxHeight = maxHeight;

    mRootNode = std::make_unique<TerrainNode>(0.0f, 0.0f, width, 0, maxHeight);
    BuildTree(mRootNode.get(), maxHeight, maxDepth);
}

void TerrainQuadTree::BuildTree(TerrainNode* node, float maxHeight, int maxDepth)
{
    if (node->Depth < maxDepth)
    {
        float halfSize = node->Size * 0.5f;
        int nextDepth = node->Depth + 1;

        node->Children[0] = std::make_unique<TerrainNode>(node->X, node->Z, halfSize, nextDepth, maxHeight);
        node->Children[1] = std::make_unique<TerrainNode>(node->X + halfSize, node->Z, halfSize, nextDepth, maxHeight);
        node->Children[2] = std::make_unique<TerrainNode>(node->X, node->Z + halfSize, halfSize, nextDepth, maxHeight);
        node->Children[3] = std::make_unique<TerrainNode>(node->X + halfSize, node->Z + halfSize, halfSize, nextDepth, maxHeight);

        BuildTree(node->Children[0].get(), maxHeight, maxDepth);
        BuildTree(node->Children[1].get(), maxHeight, maxDepth);
        BuildTree(node->Children[2].get(), maxHeight, maxDepth);
        BuildTree(node->Children[3].get(), maxHeight, maxDepth);
    }
}

void TerrainQuadTree::Update(CameraComponent* camera, FXMMATRIX terrainWorldMatrix)
{
    mDrawList.clear();

    const BoundingFrustum& frustum = camera->GetFrustumWS();
    XMFLOAT3 camPos = camera->GetPosition();

    if (mRootNode)
    {
        UpdateNode(mRootNode.get(), frustum, camPos, terrainWorldMatrix);
    }
}

void TerrainQuadTree::UpdateNode(TerrainNode* node, const BoundingFrustum& frustum, const XMFLOAT3& camPos, FXMMATRIX terrainWorldMatrix)
{
    BoundingBox worldBox;
    node->LocalAABB.Transform(worldBox, terrainWorldMatrix);

    if (frustum.Intersects(worldBox) == false)
    {
        return;
    }

    XMVECTOR nodeCenterWorld = XMVector3TransformCoord(XMLoadFloat3(&node->LocalAABB.Center), terrainWorldMatrix);
    float dist = XMVectorGetX(XMVector3Length(nodeCenterWorld - XMLoadFloat3(&camPos)));

    float splitDistance = node->Size * 2.0f;

    if (dist < splitDistance && !node->IsLeaf())
    {
        for (auto& child : node->Children)
        {
            UpdateNode(child.get(), frustum, camPos, terrainWorldMatrix);
        }
    }
    else
    {
        TerrainInstanceData data;

        data.InstancePos = XMFLOAT2(node->X, node->Z);
        data.Scale = node->Size;
        data.LOD = static_cast<float>(node->Depth);

        mDrawList.push_back(data);
    }
}