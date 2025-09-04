#include "pch.h"
#include "RendererManager.h"
#include "../Components/MeshRendererComponent.h"
#include "../Components/CameraComponent.h"
#include "../Components/TransformComponent.h"
#include "../Core/Entity.h"


void RendererManager::Register(MeshRendererComponent* comp) {
    mMeshRenderers.push_back(comp);
}

void RendererManager::RegisterCamera(CameraComponent* cam) {
    mCameras.push_back(cam);
}

void RendererManager::RenderAll() {
    for (auto* mr : mMeshRenderers) {
        auto* transform = mr->GetOwner()->GetComponent<TransformComponent>();
        if (transform) {
            mr->Render();
        }
    }
}
