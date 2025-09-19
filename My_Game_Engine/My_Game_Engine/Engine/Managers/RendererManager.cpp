#include "pch.h"
#include "RendererManager.h"
#include "../Components/MeshRendererComponent.h"
#include "../Components/CameraComponent.h"
#include "../Components/TransformComponent.h"
#include "../Core/Object.h"


void RendererManager::Register(std::weak_ptr<Component> comp) 
{
    if (auto c = comp.lock()) 
        if (auto mr = std::dynamic_pointer_cast<MeshRendererComponent>(c)) 
            mMeshRenderers.push_back(mr);

}