#include "ComponentRegistry.h"
#include "GameEngine.h"


void ComponentRegistry::Notify(std::weak_ptr<Component> comp)
{
    if (auto c = comp.lock())
    {
        switch (c->GetType())
        {
        case Component_Type::Mesh_Renderer:
        {
            if (auto mr = std::dynamic_pointer_cast<MeshRendererComponent>(c))
            {
                GameEngine::Get().GetRendererManager()->Register(mr);

                if (auto active_scene = SceneManager::Get().GetActiveScene())
                    active_scene->RegisterRenderable(mr);

            }
        }
        break;

        case Component_Type::Collider:
        case Component_Type::Rigidbody:
        {
            if (auto owner = c->GetOwner()) 
                if (auto active_scene = SceneManager::Get().GetActiveScene()) 
                    GameEngine::Get().GetPhysicsSystem()->Register(active_scene->GetId(), owner);
        }
        break;

        case Component_Type::Camera:
        {
            if (auto mr = std::dynamic_pointer_cast<CameraComponent>(c))
            {
                if (auto active_scene = SceneManager::Get().GetActiveScene())
                    active_scene->RegisterCamera(mr);
            }            
        }
            break;

        default:
            break;
        }
    }
}
