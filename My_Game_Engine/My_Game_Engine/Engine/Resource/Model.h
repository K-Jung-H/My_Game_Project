#pragma once
#include "pch.h"
#include "Mesh.h"


class Model : public Game_Resource
{
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

private:
    std::shared_ptr<Node> root;
    std::vector<std::shared_ptr<Mesh>> meshes;       // 전체 Mesh 목록
};