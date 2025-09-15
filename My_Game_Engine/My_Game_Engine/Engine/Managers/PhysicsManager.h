#pragma once
#include <vector>

class Component;
class ColliderComponent;

class PhysicsManager 
{
public:
    void Register(std::weak_ptr<Component> col);
    void Simulate(float dt);

private:
    std::vector< std::weak_ptr<ColliderComponent>> mColliders;
};
