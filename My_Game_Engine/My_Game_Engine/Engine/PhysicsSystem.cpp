#include "PhysicsSystem.h"
#include "Core/Object.h"
#include "Components/TransformComponent.h"
#include "Components/RigidbodyComponent.h"
#include "Components/ColliderComponent.h"

void PhysicsSystem::Register(SceneID id, std::shared_ptr<Object> obj)
{
    auto rb = obj->GetComponent<RigidbodyComponent>(Component_Type::Rigidbody);
    auto col = obj->GetComponent<ColliderComponent>(Component_Type::Collider);
    auto tf = obj->GetTransform();

    if (rb) 
    {
        Entry e;
        e.rb = rb;
        e.col = col; 
        e.tf = tf;
        worlds[id].dynamics.push_back(e);
    }
    else if (col) 
    {
        Entry e;
        e.col = col;
        e.tf = tf;
        worlds[id].statics.push_back(e);
    }
}

void PhysicsSystem::Unregister(SceneID id, std::shared_ptr<Object> obj)
{
    auto rb = obj->GetComponent<RigidbodyComponent>(Component_Type::Rigidbody);
    auto col = obj->GetComponent<ColliderComponent>(Component_Type::Collider);

    auto& world = worlds[id];

    if (rb) {
        world.dynamics.erase(
            std::remove_if(world.dynamics.begin(), world.dynamics.end(),
                [&](const Entry& e) {
                    auto er = e.rb.lock();
                    return er && er == rb;
                }),
            world.dynamics.end()
        );
    }

    if (col && !rb) {
        world.statics.erase(
            std::remove_if(world.statics.begin(), world.statics.end(),
                [&](const Entry& e) {
                    auto ec = e.col.lock();
                    return ec && ec == col;
                }),
            world.statics.end()
        );
    }
}

void PhysicsSystem::Update(SceneID id, float dt) 
{
    auto& w = worlds[id];

    for (auto& e : w.dynamics) 
    {
        if (auto rb = e.rb.lock()) 
        {
            if (auto tf = e.tf.lock()) 
            {
                rb->Update(dt);
                tf->ApplyMovePhysics(rb->GetVelocity(), rb->GetAngularVelocity(), dt);
            }
        }
    }
}

void PhysicsSystem::Clear(SceneID id) 
{
    worlds.erase(id);
}