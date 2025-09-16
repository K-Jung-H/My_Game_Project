#include "pch.h"
#include "ComponentRegistry.h"
#include "GameEngine.h"

void ComponentRegistry::Notify(std::weak_ptr<Component> comp)
{
    if (auto c = comp.lock())
    {
        switch (c->GetType())
        {
        case Component_Type::Mesh_Renderer:
            GameEngine::Get().GetRendererManager()->Register(comp); 
            break;

        case Component_Type::Collider:
            GameEngine::Get().GetPhysicsManager()->Register(comp);
            break;

        case Component_Type::Camera:
            GameEngine::Get().GetRendererManager()->RegisterCamera(comp);
            break;

        default:
            break;
        }
    }
}
