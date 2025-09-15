#include "pch.h"
#include "PhysicsManager.h"
#include "../Components/ColliderComponent.h"


void PhysicsManager::Register(std::weak_ptr<Component> com) 
{
    if (auto c = com.lock())
        if (auto col = std::dynamic_pointer_cast<ColliderComponent>(c))
            mColliders.push_back(col);
}

void PhysicsManager::Simulate(float dt) 
{
    for (auto it = mColliders.begin(); it != mColliders.end(); )
    {
        if (auto col = it->lock())
        {
            std::cout << "Physics step - collider radius: " << col->GetRadius() << "\n";

            ++it;
        }
        else
        {
            it = mColliders.erase(it);
        }

    }


}
