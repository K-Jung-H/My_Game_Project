#include "pch.h"
#include "PhysicsManager.h"
#include "../Components/ColliderComponent.h"


void PhysicsManager::Register(ColliderComponent* col) {
    mColliders.push_back(col);
}

void PhysicsManager::Simulate(float dt) {
    for (auto* col : mColliders) {
        std::cout << "Physics step - collider radius: " << col->GetRadius() << "\n";
    }
}
