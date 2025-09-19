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
