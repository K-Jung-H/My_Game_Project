#pragma once
#include "Game_Resource.h"

enum class VertexFlags : UINT
{
    None = 0,
    HasNormal = 1 << 0,
    HasTangent = 1 << 1,
    HasUV0 = 1 << 2,
    HasUV1 = 1 << 5,
    HasColor0 = 1 << 3,
    Skinned = 1 << 4,
};

inline VertexFlags operator|(VertexFlags a, VertexFlags b) { return (VertexFlags)((UINT)a | (UINT)b); }
inline VertexFlags& operator|=(VertexFlags& a, VertexFlags b) { a = a | b; return a; }
inline bool HasFlag(VertexFlags f, VertexFlags bit) { return (((UINT)f & (UINT)bit) != 0); }

struct VertexStreamLayout
{
    struct Attr { UINT offset = 0; DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN; bool present = false; };
    UINT stride = 0;
    Attr position;
    Attr normal;
    Attr tangent;
    Attr uv0;
    Attr uv1;
    Attr color0;
};

class Skeleton;
static const int MAX_BONES_PER_VERTEX = 4;

class Mesh : public Game_Resource
{
    friend class ModelLoader_Assimp;
    friend class ModelLoader_FBX;

public:
    struct Submesh
    {
        UINT indexCount = 0;
        UINT startIndexLocation = 0;
        INT  baseVertexLocation = 0;
        UINT materialId = Engine::INVALID_ID;
        BoundingBox localAABB;
    };

    Mesh();
    virtual ~Mesh() = default;
    virtual bool LoadFromFile(std::string path, const RendererContext& ctx);
    virtual void FromAssimp(const aiMesh* mesh);
    virtual void FromFbxSDK(FbxMesh* fbxMesh);
    virtual void Bind(ComPtr<ID3D12GraphicsCommandList> cmdList, UINT current_frame) const;

    VertexFlags GetVertexFlags() const { return mVertexFlags; }
    const VertexStreamLayout& GetHotLayout() const { return mHotLayout; }
    const VertexStreamLayout& GetColdLayout() const { return mColdLayout; }
    UINT GetIndexCount() const { return static_cast<UINT>(indices.size()); }
    UINT GetMaterialID() const { return submeshes.empty() ? Engine::INVALID_ID : submeshes[0].materialId; }
    const BoundingBox& GetLocalAABB() const { return mLocalAABB; }
protected:
    void BuildInterleavedBuffers();
    void UploadIndexBuffer();
    void SetAABB();

protected:
    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT3> normals;
    std::vector<XMFLOAT3> tangents;
    std::vector<XMFLOAT2> uvs;
    std::vector<XMFLOAT2> uv1s;
    std::vector<XMFLOAT4> colors;
    std::vector<UINT>     indices;

    std::vector<uint8_t>  mHotCPU;
    std::vector<uint8_t>  mColdCPU;

    VertexFlags           mVertexFlags = VertexFlags::None;
    VertexStreamLayout    mHotLayout{};
    VertexStreamLayout    mColdLayout{};

    ComPtr<ID3D12Resource> mHotVB;
    ComPtr<ID3D12Resource> mColdVB;
    ComPtr<ID3D12Resource> mIndexBuffer;

    ComPtr<ID3D12Resource> mHotUpload;
    ComPtr<ID3D12Resource> mColdUpload;
    ComPtr<ID3D12Resource> mIndexUpload;

    D3D12_VERTEX_BUFFER_VIEW mHotVBV{};
    D3D12_VERTEX_BUFFER_VIEW mColdVBV{};
    D3D12_INDEX_BUFFER_VIEW  mIBV{};

    BoundingBox mLocalAABB;

public:
    std::vector<Submesh> submeshes;
};

class Plane_Mesh : public Mesh
{
public:
    Plane_Mesh(float width = 1.0f, float height = 1.0f);
    virtual ~Plane_Mesh() = default;

private:
    void GeneratePlane(float width, float height);
};

struct GPU_SkinData
{
    uint16_t idx[MAX_BONES_PER_VERTEX];
    uint16_t w16[MAX_BONES_PER_VERTEX];
};

class SkinnedMesh : public Mesh
{
public:
    struct VertexBoneDataCPU
    {
        uint16_t boneIndices[MAX_BONES_PER_VERTEX] = { 0,0,0,0 };
        float    weights[MAX_BONES_PER_VERTEX] = { 0,0,0,0 };
    };

    struct BoneMappingData
    {
        std::string boneName;
        unsigned int vertexId;
        float weight;
    };

    struct FrameSkinBuffer
    {
        ComPtr<ID3D12Resource> skinnedBuffer;
        ComPtr<ID3D12Resource> uploadBuffer;
        D3D12_VERTEX_BUFFER_VIEW vbv;
        UINT uavSlot = UINT_MAX;
        UINT srvSlot = UINT_MAX;

        bool mIsSkinningResultReady = false;
    };


    void Skinning_Skeleton_Bones(const Skeleton& skeleton);
    void CreatePreSkinnedOutputBuffer();

    virtual void FromAssimp(const aiMesh* mesh) override;
    virtual void FromFbxSDK(FbxMesh* fbxMesh) override;
    virtual void Bind(ComPtr<ID3D12GraphicsCommandList> cmdList, UINT current_frame) const override;

    D3D12_GPU_VIRTUAL_ADDRESS GetHotInputGPUVA() const { return mHotVB ? mHotVB->GetGPUVirtualAddress() : 0; }
    D3D12_GPU_VIRTUAL_ADDRESS GetSkinDataGPUVA() const { return mSkinData ? mSkinData->GetGPUVirtualAddress() : 0; }

    UINT GetVertexCount() const { return static_cast<UINT>(positions.size()); }
    UINT GetHotStride() const { return mHotLayout.stride; }
	bool IsSkinningBufferExisted() const { return mHasSkinnedBuffer; }
    FrameSkinBuffer& GetFrameSkinBuffer(UINT frameIndex)  { return mFrameSkinnedBuffers[frameIndex]; }

    UINT SkinDataSRV = UINT_MAX;
    UINT HotInputSRV = UINT_MAX;

    std::vector<VertexBoneDataCPU> bone_vertex_data;
    std::vector<BoneMappingData>   bone_mapping_data;

protected:
    ComPtr<ID3D12Resource> mSkinData;
    ComPtr<ID3D12Resource> mSkinDataUpload;

    FrameSkinBuffer mFrameSkinnedBuffers[Engine::Frame_Render_Buffer_Count];
    bool mHasSkinnedBuffer = false;
};
