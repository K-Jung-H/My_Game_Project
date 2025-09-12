#pragma once
#include "Game_Resource.h"


class Mesh : public Game_Resource
{
public:
    Mesh();
    virtual ~Mesh() = default;
    virtual bool LoadFromFile(std::string_view path, const RendererContext& ctx);

    void FromAssimp(const aiMesh* mesh);

    UINT GetSlot() const { return -1; }
public:
    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT3> normals;
    std::vector<XMFLOAT3> tangents;
    std::vector<XMFLOAT2> uvs;
    std::vector<XMFLOAT4> colors;

    std::vector<uint32_t> indices;

    UINT materialIndex = 0;

};