#include "pch.h"
#include "MeshRendererComponent.h"
#include "GameEngine.h"


void MeshRendererComponent::SetMesh(UINT id)
{
    meshId = id;
    auto rc = GameEngine::Get().GetResourceManager();
    
    mMesh = rc->GetById<Mesh>(id);
}

void MeshRendererComponent::SetMaterial(UINT id)
{
    materialId = id;
    auto rc = GameEngine::Get().GetResourceManager();

    mMaterial = rc->GetById<Material>(id);
}

void MeshRendererComponent::Render() 
{
    // TODO: DX12 rendering
    // 1. Bind pipeline state & root signature
    // 2. Set vertex/index buffer from mMesh
    // 3. Set material descriptors
    // 4. Issue DrawIndexedInstanced

    std::cout << "Rendering MeshRendererComponent (stub)\n";
}
