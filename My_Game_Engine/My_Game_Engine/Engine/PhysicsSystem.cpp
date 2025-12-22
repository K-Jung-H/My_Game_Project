#include "PhysicsSystem.h"
#include "Core/Object.h"
#include "Components/TransformComponent.h"
#include "Components/RigidbodyComponent.h"
#include "Components/ColliderComponent.h"
#include "Components/TerrainComponent.h"

void PhysicsSystem::Register(SceneID id, Object* obj)
{
    auto rb = obj->GetComponent<RigidbodyComponent>();
    auto col = obj->GetComponent<ColliderComponent>();
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

void PhysicsSystem::Unregister(SceneID id, Object* obj)
{
    auto rb = obj->GetComponent<RigidbodyComponent>();
    auto col = obj->GetComponent<ColliderComponent>();

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

void PhysicsSystem::RegisterTerrain(SceneID id, TerrainComponent* terrain)
{
    auto& list = worlds[id].terrains;
    if (std::find(list.begin(), list.end(), terrain) == list.end())
    {
        list.push_back(terrain);
    }
}

void PhysicsSystem::UnregisterTerrain(SceneID id, TerrainComponent* terrain)
{
    auto& list = worlds[id].terrains;
    auto it = std::remove(list.begin(), list.end(), terrain);
    list.erase(it, list.end());
}

void PhysicsSystem::Update(SceneID id, float dt)
{
    Update_Integration(id, dt);
    Update_Object_Terrain_Interact(id, dt);
    Update_Object_Object_Interact(id, dt);
}

void PhysicsSystem::Update_Integration(SceneID id, float dt)
{
    auto& world = worlds[id];

    for (auto& entry : world.dynamics)
    {
        auto tf = entry.tf.lock();
        auto rb = entry.rb.lock();

        if (!tf || !rb) continue;

        rb->Update(dt);

        XMFLOAT3 currentPos = tf->GetPosition();
        XMFLOAT3 velocity = rb->GetVelocity();

        currentPos.x += velocity.x * dt;
        currentPos.y += velocity.y * dt;
        currentPos.z += velocity.z * dt;

        tf->SetPosition(currentPos);
    }
}

void PhysicsSystem::Update_Object_Terrain_Interact(SceneID id, float dt)
{
    auto& world = worlds[id];
    const auto& terrains = world.terrains;

    if (terrains.empty()) return;

    for (auto& entry : world.dynamics)
    {
        auto tf = entry.tf.lock();
        auto rb = entry.rb.lock();
        auto col = entry.col.lock();

        if (!tf || !rb) continue;

        XMFLOAT3 currentPos = tf->GetPosition();
        XMFLOAT3 velocity = rb->GetVelocity();

        float maxTerrainHeight = -FLT_MAX;
        bool isOverTerrain = false;

        float colliderHalfHeight = 0.0f;
        if (col)
        {
            colliderHalfHeight = 1.0f;
        }

        float objectBottomY = currentPos.y - colliderHalfHeight;

        for (auto* terrain : terrains)
        {
            float h = terrain->GetHeight(currentPos);

            if (h > -FLT_MAX && h > maxTerrainHeight)
            {
                maxTerrainHeight = h;
                isOverTerrain = true;
            }
        }

        if (isOverTerrain)
        {
            if (objectBottomY < maxTerrainHeight)
            {
                currentPos.y = maxTerrainHeight + colliderHalfHeight;
                tf->SetPosition(currentPos);

                if (velocity.y < 0.0f)
                {
                    velocity.y = 0.0f;
                    rb->SetVelocity(velocity);
                }
            }
        }
    }
}

void PhysicsSystem::Update_Object_Object_Interact(SceneID id, float dt)
{
    // 추후 구현: 동적 오브젝트 간의 충돌 처리 (Sphere-Sphere, AABB-AABB 등)
    // auto& world = worlds[id];
    // for (auto& objA : world.dynamics) { ... }
}

void PhysicsSystem::Clear(SceneID id) 
{
    worlds.erase(id);
}