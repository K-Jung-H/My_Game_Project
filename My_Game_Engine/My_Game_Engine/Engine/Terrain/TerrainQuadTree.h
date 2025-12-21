#pragma once
#include "TerrainCommon.h"

class CameraComponent;

struct TerrainNode
{
    BoundingBox LocalAABB;
    float X;
    float Z;
    float Size;
    int   Depth;
    std::array<std::unique_ptr<TerrainNode>, 4> Children;

    bool IsLeaf() const { return Children[0] == nullptr; }

    TerrainNode(float x, float z, float size, int depth, float maxHeight);
};

class TerrainQuadTree
{
public:
    TerrainQuadTree();
    ~TerrainQuadTree();

    void Initialize(float width, float depth, float maxHeight, int maxDepth);
    void Update(CameraComponent* camera, DirectX::FXMMATRIX terrainWorldMatrix);

    const std::vector<TerrainInstanceData>& GetDrawList() const { return mDrawList; }

private:
    void BuildTree(TerrainNode* node, float maxHeight, int maxDepth);
    void UpdateNode(TerrainNode* node, const DirectX::BoundingFrustum& frustum, const DirectX::XMFLOAT3& camPos, DirectX::FXMMATRIX terrainWorldMatrix);

private:
    std::unique_ptr<TerrainNode> mRootNode;
    std::vector<TerrainInstanceData> mDrawList;

    float mMaxHeight = 0.0f;
};