#pragma once
#include "Game_Resource.h"

class Skeleton;
static const int MAX_BONES_PER_VERTEX = 4;



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

    virtual  void FromAssimp(const aiMesh* mesh);

    UINT GetSlot() const { return -1; }
    UINT GetIndexCount() const { return static_cast<UINT>(indices.size()); }

public:
    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT3> normals;
    std::vector<XMFLOAT3> tangents;
    std::vector<XMFLOAT2> uvs;
    std::vector<XMFLOAT4> colors;

    std::vector<UINT> indices;

    UINT materialIndex = 0;
    std::vector<Submesh> submeshes;
};




class SkinnedMesh : public Mesh
{
public:
    struct BoneMappingData
    {
        std::string boneName;
        unsigned int vertexId;
        float weight;
    };

    struct VertexBoneData
    {
        UINT boneIndices[MAX_BONES_PER_VERTEX] = { 0,0,0,0 };
        float weights[MAX_BONES_PER_VERTEX] = { 0.f,0.f,0.f,0.f };
    };

    virtual void FromAssimp(const aiMesh* mesh);
    void Skinning_Skeleton_Bones(const Skeleton& skeleton);

public:
    std::vector< VertexBoneData> bone_vertex_data;
    std::vector< BoneMappingData> bone_mapping_data;

protected:
    bool is_skinned_bones = false;

};