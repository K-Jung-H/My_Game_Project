#pragma once
#include "Component.h"

class ComponentRegistry 
{
public:
    void Notify(GameEngine& engine, Component* comp);
};
