#pragma once
#include "Mesh.h"
#include "Skeleton.h"

class Model : public Game_Resource
{
public:
    static bool loadAndExport(const std::string& fbxPath, const std::string& outTxt = "mesh_texture_list.txt");

public:
    static UINT CountNodes(const std::shared_ptr<Model>& model);

public:
    struct Node
    {
        std::string name;
        XMFLOAT4X4  localTransform;
        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<std::shared_ptr<Node>> children;

        Node() { XMStoreFloat4x4(&localTransform, XMMatrixIdentity()); }
    };

public:
    Model();
    virtual ~Model() = default;

    virtual bool LoadFromFile(std::string path, const RendererContext& ctx) { return false; }
    virtual bool SaveToFile(const std::string& outputPath) const { return false; }

    std::shared_ptr<Node> GetRoot() const { return root; }
    void SetRoot(const std::shared_ptr<Node>& node) { root = node; }

    std::shared_ptr<Skeleton> GetSkeleton() const { return mModelSkeleton; }
	void SetSkeleton(std::shared_ptr<Skeleton> s) { mModelSkeleton = s; }

    void SetAvatarID(UINT id) { mAvatarID = id; }
    UINT GetAvatarID() const { return mAvatarID; }

    UINT GetMeshCount() const { return mMeshCount; }
    void SetMeshCount(UINT count) { mMeshCount = count; }


    UINT GetMaterialCount() const { return mMaterialCount; }
    void SetMaterialCount(UINT count) { mMaterialCount = count; }

    UINT GetTextureCount() const { return mTextureCount; }
    void SetTextureCount(UINT count) { mTextureCount = count; }

private:
    std::shared_ptr<Node> root;

    std::shared_ptr<Skeleton> mModelSkeleton;
    UINT mAvatarID = Engine::INVALID_ID;

	UINT mMeshCount = 0;
    UINT mMaterialCount = 0;
    UINT mTextureCount = 0;
};
