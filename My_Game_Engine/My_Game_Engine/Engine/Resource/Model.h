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

    std::shared_ptr<Node> GetRoot() const { return root; }
    void SetRoot(const std::shared_ptr<Node>& node) { root = node; }

    const std::vector<std::shared_ptr<Mesh>>& GetAllMeshes() const { return meshes; }
    void AddMesh(const std::shared_ptr<Mesh>& mesh) { meshes.push_back(mesh); }

    virtual bool LoadFromFile(std::string path, const RendererContext& ctx);

	std::shared_ptr<Skeleton> GetSkeleton() const { return mSkeleton; }
	void SetSkeleton(std::shared_ptr<Skeleton> s) { mSkeleton = s; }

private:
    std::shared_ptr<Node> root;
    std::vector<std::shared_ptr<Mesh>> meshes;       // 전체 Mesh 목록

    std::shared_ptr<Skeleton> mSkeleton;
};
