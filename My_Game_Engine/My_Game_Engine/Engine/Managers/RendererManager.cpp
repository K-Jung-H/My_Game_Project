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

void RendererManager::RegisterCamera(std::weak_ptr <Component> cam)
{
    if (auto c = cam.lock())
        if (auto mr = std::dynamic_pointer_cast<CameraComponent>(c))
            mCameras.push_back(mr);

}

void RendererManager::RenderAll()
{
    for (auto it = mMeshRenderers.begin(); it != mMeshRenderers.end(); )
    {
        if (auto mr = it->lock()) 
        {
            if (auto owner = mr->GetOwner())
                if (auto transform = owner->GetComponent<TransformComponent>(Component_Type::Transform))
                    mr->Render();
            ++it;
        }
        else
            it = mMeshRenderers.erase(it);

    }
}