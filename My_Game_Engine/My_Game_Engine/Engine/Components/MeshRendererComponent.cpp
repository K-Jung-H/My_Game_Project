#include "pch.h"
#include "MeshRendererComponent.h"

MeshRendererComponent::MeshRendererComponent() {
}

void MeshRendererComponent::SetMesh(Mesh* mesh) {
    mMesh = mesh;
}

void MeshRendererComponent::SetMaterial(Material* material) {
    mMaterial = material;
}

void MeshRendererComponent::Render() {
    // TODO: DX12 rendering
    // 1. Bind pipeline state & root signature
    // 2. Set vertex/index buffer from mMesh
    // 3. Set material descriptors
    // 4. Issue DrawIndexedInstanced

    std::cout << "Rendering MeshRendererComponent (stub)\n";
}
