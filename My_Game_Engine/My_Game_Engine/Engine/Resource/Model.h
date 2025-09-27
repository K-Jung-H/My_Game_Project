#pragma once
#include "pch.h"
#include "Mesh.h"

struct Bone
{
    std::string name;
    int parentIndex;
    XMFLOAT4X4 inverseBind; 
};

struct Skeleton
{
    std::vector<Bone> BoneList;
    std::unordered_set<std::string> BoneNames;

    std::unordered_map<std::string, int> NameToIndex;

    void BuildNameToIndex()
    {
        NameToIndex.clear();
        for (size_t i = 0; i < BoneList.size(); i++)
            NameToIndex[BoneList[i].name] = static_cast<int>(i);
    }
};


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



    virtual bool LoadFromFile(std::string_view path, const RendererContext& ctx);

    void SetSkeleton(Skeleton& s) { skeleton = s; }
    const Skeleton& GetSkeleton() const { return skeleton; }

private:
    std::shared_ptr<Node> root;
    std::vector<std::shared_ptr<Mesh>> meshes;       // 전체 Mesh 목록
    Skeleton skeleton;
};
