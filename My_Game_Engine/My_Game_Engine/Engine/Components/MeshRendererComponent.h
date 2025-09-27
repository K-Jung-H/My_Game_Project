#pragma once
#include "../Core/Component.h"

class Mesh;
class Material;

class MeshRendererComponent : public Component
{
public:
    static constexpr Component_Type Type = Component_Type::Mesh_Renderer;
    Component_Type GetType() const override { return Type; }

public:
    MeshRendererComponent() = default;
    ~MeshRendererComponent() = default;

    void SetMesh(UINT id);
    UINT GetMeshId() const { return meshId; }

    std::shared_ptr<Mesh> GetMesh() const { return mMesh.lock(); }

private:
    UINT meshId = Engine::INVALID_ID;

    std::weak_ptr<Mesh> mMesh;
};