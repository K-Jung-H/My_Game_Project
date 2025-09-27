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
    UINT GetMaterialID() const { return submeshes[0].materialId;; }

    void Bind(ComPtr<ID3D12GraphicsCommandList> cmdList) const;

public:
    std::vector<Submesh> submeshes;

    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT3> normals;
    std::vector<XMFLOAT3> tangents;
    std::vector<XMFLOAT2> uvs;
    std::vector<XMFLOAT4> colors;

    std::vector<UINT> indices;


private:

    ComPtr<ID3D12Resource> posBuffer;
    ComPtr<ID3D12Resource> normalBuffer;
    ComPtr<ID3D12Resource> tangentBuffer;
    ComPtr<ID3D12Resource> uvBuffer;
    ComPtr<ID3D12Resource> colorBuffer;
    ComPtr<ID3D12Resource> indexBuffer;

    D3D12_VERTEX_BUFFER_VIEW posVBV{};
    D3D12_VERTEX_BUFFER_VIEW normalVBV{};
    D3D12_VERTEX_BUFFER_VIEW tangentVBV{};
    D3D12_VERTEX_BUFFER_VIEW uvVBV{};
    D3D12_VERTEX_BUFFER_VIEW colorVBV{};
    D3D12_INDEX_BUFFER_VIEW  indexView{};

    ComPtr<ID3D12Resource> posUpload;
    ComPtr<ID3D12Resource> normalUpload;
    ComPtr<ID3D12Resource> tangentUpload;
    ComPtr<ID3D12Resource> uvUpload;
    ComPtr<ID3D12Resource> colorUpload;
    ComPtr<ID3D12Resource> indexUpload;
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