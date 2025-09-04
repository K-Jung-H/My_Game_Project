#pragma once
#include <vector>
class ColliderComponent;

class PhysicsManager {
public:
    void Register(ColliderComponent* col);
    void Simulate(float dt);

private:
    std::vector<ColliderComponent*> mColliders;
};
