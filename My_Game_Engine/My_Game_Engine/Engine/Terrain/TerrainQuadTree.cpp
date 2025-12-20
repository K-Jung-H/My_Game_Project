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
    XMVECTOR scale, rot, trans;
    XMMatrixDecompose(&scale, &rot, &trans, terrainWorldMatrix);

    float worldScale = XMVectorGetX(scale);
    float worldNodeSize = node->Size * worldScale;

    BoundingBox worldBox;
    node->LocalAABB.Transform(worldBox, terrainWorldMatrix);

    if (frustum.Intersects(worldBox) == false)
    {
        return;
    }

    XMVECTOR camVec = XMLoadFloat3(&camPos);
    XMVECTOR boxCenter = XMLoadFloat3(&worldBox.Center);
    XMVECTOR boxExtents = XMLoadFloat3(&worldBox.Extents);

    XMVECTOR boxMin = boxCenter - boxExtents;
    XMVECTOR boxMax = boxCenter + boxExtents;

    XMVECTOR closestPoint = XMVectorClamp(camVec, boxMin, boxMax);
    float dist = XMVectorGetX(XMVector3Length(camVec - closestPoint));


    float splitDistance = worldNodeSize * 2.0f;

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

        const float detailScale = 100.0f;
        const float falloffPower = 2.0f;

        const float minTessFactor = 4.0f;
        const float maxTessFactor = 64.0f;

        float denominator = std::pow(dist + 1.0f, falloffPower);
        float tessValue = (worldNodeSize * detailScale) / denominator;

        data.LOD = std::clamp(tessValue, minTessFactor, maxTessFactor);

        mDrawList.push_back(data);
    }
}