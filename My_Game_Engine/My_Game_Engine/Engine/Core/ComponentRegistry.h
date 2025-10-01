#pragma once
#include "Component.h"

class ComponentRegistry 
{
public:
    static void Notify(std::weak_ptr<Component> comp);
};
