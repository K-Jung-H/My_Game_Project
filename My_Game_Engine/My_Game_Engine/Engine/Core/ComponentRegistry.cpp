#include "pch.h"
#include "ComponentRegistry.h"
#include "../Components/MeshRendererComponent.h"
#include "../Components/ColliderComponent.h"
#include "../Components/CameraComponent.h"
#include "../Managers/RendererManager.h"
#include "../Managers/PhysicsManager.h"

void ComponentRegistry::Notify(GameEngine& engine, Component* comp) {
    if (auto* mr = dynamic_cast<MeshRendererComponent*>(comp))
        engine.GetRendererManager()->Register(mr);

    if (auto* col = dynamic_cast<ColliderComponent*>(comp))
        engine.GetPhysicsManager()->Register(col);

    if (auto* cam = dynamic_cast<CameraComponent*>(comp))
        engine.GetRendererManager()->RegisterCamera(cam);
}
