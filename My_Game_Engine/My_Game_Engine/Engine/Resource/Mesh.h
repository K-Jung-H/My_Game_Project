#pragma once
#include "Game_Resource.h"

class Mesh : public Game_Resource
{
public:
    struct Submesh
    {
        UINT indexCount = 0;
        UINT startIndexLocation = 0;
        INT  baseVertexLocation = 0;
        UINT materialId = Engine::INVALID_ID;
    };
    
public:
    Mesh();
    virtual ~Mesh() = default;
    virtual bool LoadFromFile(std::string_view path, const RendererContext& ctx);

    void FromAssimp(const aiMesh* mesh);

    UINT GetSlot() const { return -1; }
    UINT GetIndexCount() const { return static_cast<UINT>(indices.size()); }

public:
    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT3> normals;
    std::vector<XMFLOAT3> tangents;
    std::vector<XMFLOAT2> uvs;
    std::vector<XMFLOAT4> colors;

    std::vector<uint32_t> indices;


    UINT materialIndex = 0;
    std::vector<Submesh> submeshes;
};

