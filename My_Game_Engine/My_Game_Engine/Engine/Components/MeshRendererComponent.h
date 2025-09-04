#pragma once
#include "../Core/Component.h"

// Forward declaration
class Mesh;
class Material;

class MeshRendererComponent : public Component {
public:
    MeshRendererComponent();

    void SetMesh(Mesh* mesh);
    void SetMaterial(Material* material);

    virtual void Render() override;

private:
    Mesh* mMesh = nullptr;         // Pointer to mesh resource
    Material* mMaterial = nullptr; // Pointer to material/shader resource
};
