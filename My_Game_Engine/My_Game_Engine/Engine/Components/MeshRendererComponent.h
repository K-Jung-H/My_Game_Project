#pragma once
#include "Core/Component.h"

class Mesh;

class MeshRendererComponent : public SynchronizedComponent
{
public:
    virtual rapidjson::Value ToJSON(rapidjson::Document::AllocatorType& alloc) const;
    virtual void FromJSON(const rapidjson::Value& val);

public:
    static constexpr Component_Type Type = Component_Type::Mesh_Renderer;
    Component_Type GetType() const override { return Type; }

public:
    MeshRendererComponent() = default;
    ~MeshRendererComponent() = default;

    virtual void SetMesh(UINT id);
    std::shared_ptr<Mesh> GetMesh() const { return mMesh.lock(); }
    UINT GetMeshId() const { return meshId; }

    void SetMaterial(size_t submeshIndex, UINT materialId);
    UINT GetMaterial(size_t submeshIndex) const;

private:
    UINT meshId = Engine::INVALID_ID;

    std::weak_ptr<Mesh> mMesh;

    std::vector<UINT> materialOverrides;
};