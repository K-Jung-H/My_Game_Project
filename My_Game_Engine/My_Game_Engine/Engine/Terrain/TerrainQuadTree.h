#pragma once
#include "TerrainCommon.h"

struct TerrainNode
{
    BoundingBox LocalAABB;
    float X;
    float Z;
    float Size;
    int   Depth;
    std::array<std::unique_ptr<TerrainNode>, 4> Children;

    bool IsLeaf() const { return Children[0] == nullptr; }

    TerrainNode(float x, float z, float size, int depth)
        : X(x), Z(z), Size(size), Depth(depth) {
    }
};

class TerrainQuadTree
{
public:
    TerrainQuadTree();
    ~TerrainQuadTree();

    void Initialize(float mapWidth, float mapDepth, float maxHeight, int maxDepth);
    void Update(XMFLOAT3 cameraWorldPos, const XMMATRIX& terrainWorldMatrix);

    const std::vector<TerrainInstanceData>& GetDrawList() const { return mDrawList; }

private:
    void BuildRecursive(TerrainNode* node);
    void CullAndSelectLOD(TerrainNode* node, XMFLOAT3 cameraLocalPos);

private:
    std::unique_ptr<TerrainNode> mRoot;
    std::vector<TerrainInstanceData> mDrawList;

    float mWidth = 0.0f;
    float mDepth = 0.0f;
    float mMaxHeight = 0.0f;
    int   mMaxDepth = 0;
    float mLODDistanceRatio = 2.0f;
};