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
    void SetMaterial(UINT id);

    UINT GetMeshId() const { return meshId; }
    UINT GetMaterialId() const { return materialId; }

    std::shared_ptr<Mesh> GetMesh() const { return mMesh.lock(); }
    std::shared_ptr<Material> GetMaterial() const { return mMaterial.lock(); }

    virtual void Render() override;

private:
    UINT meshId = Engine::INVALID_ID;
    UINT materialId = Engine::INVALID_ID;

    std::weak_ptr<Mesh> mMesh;
    std::weak_ptr<Material> mMaterial;
};