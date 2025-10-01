#include "RendererManager.h"
#include "Components/MeshRendererComponent.h"
#include "Components/TransformComponent.h"


void RendererManager::Register(std::weak_ptr<Component> comp) 
{
    if (auto c = comp.lock()) 
        if (auto mr = std::dynamic_pointer_cast<MeshRendererComponent>(c)) 
            mMeshRenderers.push_back(mr);

}