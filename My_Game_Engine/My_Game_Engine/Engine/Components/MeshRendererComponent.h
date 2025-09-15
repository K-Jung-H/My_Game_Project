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
    MeshRendererComponent();

    void SetMesh(Mesh* mesh);
    void SetMaterial(Material* material);

    virtual void Render() override;

private:
    Mesh* mMesh = nullptr;         
    Material* mMaterial = nullptr; 
};
