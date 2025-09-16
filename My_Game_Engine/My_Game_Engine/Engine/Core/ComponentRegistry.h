#pragma once
#include "Component.h"
#include "../Managers/RendererManager.h"
#include "../Managers/PhysicsManager.h"

class ComponentRegistry 
{
public:
    static void Notify(std::weak_ptr<Component> comp);
};
