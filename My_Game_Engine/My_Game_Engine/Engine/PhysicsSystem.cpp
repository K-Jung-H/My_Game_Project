#include "PhysicsSystem.h"
#include "Core/Object.h"
#include "Components/TransformComponent.h"
#include "Components/RigidbodyComponent.h"
#include "Components/ColliderComponent.h"
#include "Components/TerrainComponent.h"

namespace PhysicsUtils
{
    struct AABB { XMFLOAT3 min; XMFLOAT3 max; };

    AABB GetAABB(TransformComponent* tf, ColliderComponent* col)
    {
        XMFLOAT3 pos = tf->GetPosition();
        XMFLOAT3 center = col->GetCenter();
        XMFLOAT3 size = col->GetSize();
        XMFLOAT3 scale = tf->GetScale();

        XMFLOAT3 worldCenter = { pos.x + center.x, pos.y + center.y, pos.z + center.z };

        XMFLOAT3 extent = {
            (size.x * scale.x) * 0.5f,
            (size.y * scale.y) * 0.5f,
            (size.z * scale.z) * 0.5f
        };

        return {
            { worldCenter.x - extent.x, worldCenter.y - extent.y, worldCenter.z - extent.z },
            { worldCenter.x + extent.x, worldCenter.y + extent.y, worldCenter.z + extent.z }
        };
    }
}

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

        float distToBottom = 0.0f;
        XMFLOAT3 centerOffset = { 0.f, 0.f, 0.f };

        if (col)
        {
            centerOffset = col->GetCenter();

            switch (col->GetColliderType())
            {
            case Collider_Type::Sphere:
                distToBottom = col->GetRadius();
                break;

            case Collider_Type::Box:
                distToBottom = col->GetSize().y * 0.5f;
                break;

            case Collider_Type::Capsule:
                distToBottom = col->GetHeight() * 0.5f;
                break;

            case Collider_Type::Mesh:
            case Collider_Type::Terrain:
            default:
                // 추후 구현
                distToBottom = 0.0f;
                break;
            }
        }

        XMFLOAT3 checkPos = currentPos;
        checkPos.x += centerOffset.x;
        checkPos.z += centerOffset.z;


        float objectBottomY = (currentPos.y + centerOffset.y) - distToBottom;

        float maxTerrainHeight = -FLT_MAX;
        bool isOverTerrain = false;

        for (auto* terrain : terrains)
        {
            float h = terrain->GetHeight(checkPos);

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
                float correctedY = (maxTerrainHeight + distToBottom) - centerOffset.y;

                currentPos.y = correctedY;
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
    auto& world = worlds[id];
    auto& dynamics = world.dynamics;
    size_t count = dynamics.size();

    for (size_t i = 0; i < count; ++i)
    {
        for (size_t j = i + 1; j < count; ++j)
        {
            auto& entryA = dynamics[i];
            auto& entryB = dynamics[j];

            auto tfA = entryA.tf.lock();
            auto rbA = entryA.rb.lock();
            auto colA = entryA.col.lock();

            auto tfB = entryB.tf.lock();
            auto rbB = entryB.rb.lock();
            auto colB = entryB.col.lock();

            if (!tfA || !rbA || !colA || !tfB || !rbB || !colB) continue;

            bool isColliding = false;
            XMFLOAT3 normal = { 0, 0, 0 };
            float penetration = 0.0f;

            Collider_Type typeA = colA->GetColliderType();
            Collider_Type typeB = colB->GetColliderType();

            XMFLOAT3 posA = tfA->GetPosition();
            XMFLOAT3 posB = tfB->GetPosition();
            XMFLOAT3 offsetA = colA->GetCenter();
            XMFLOAT3 offsetB = colB->GetCenter();

            XMFLOAT3 centerA = { posA.x + offsetA.x, posA.y + offsetA.y, posA.z + offsetA.z };
            XMFLOAT3 centerB = { posB.x + offsetB.x, posB.y + offsetB.y, posB.z + offsetB.z };

            if (typeA == Collider_Type::Sphere && typeB == Collider_Type::Sphere)
            {
                float rA = colA->GetRadius(); 
                float rB = colB->GetRadius();

                float dx = centerA.x - centerB.x;
                float dy = centerA.y - centerB.y;
                float dz = centerA.z - centerB.z;
                float distSq = dx * dx + dy * dy + dz * dz;
                float radiusSum = rA + rB;

                if (distSq < radiusSum * radiusSum)
                {
                    float dist = sqrt(distSq);
                    isColliding = true;
                    penetration = radiusSum - dist;

                    if (dist > 0.0001f) 
                    {
                        normal = { dx / dist, dy / dist, dz / dist };
                    }
                    else
                    {
                        normal = { 0, 1, 0 }; 
                    }
                }
            }
            else if (typeA == Collider_Type::Box && typeB == Collider_Type::Box)
            {
                auto aabbA = PhysicsUtils::GetAABB(tfA.get(), colA.get());
                auto aabbB = PhysicsUtils::GetAABB(tfB.get(), colB.get());

                if (aabbA.max.x > aabbB.min.x && aabbA.min.x < aabbB.max.x &&
                    aabbA.max.y > aabbB.min.y && aabbA.min.y < aabbB.max.y &&
                    aabbA.max.z > aabbB.min.z && aabbA.min.z < aabbB.max.z)
                {
                    isColliding = true;

                    float overlaps[] = {
                        aabbA.max.x - aabbB.min.x,
                        aabbB.max.x - aabbA.min.x,
                        aabbA.max.y - aabbB.min.y,
                        aabbB.max.y - aabbA.min.y,
                        aabbA.max.z - aabbB.min.z,
                        aabbB.max.z - aabbA.min.z,
                    };


                    float dx = centerA.x - centerB.x;
                    float dy = centerA.y - centerB.y;
                    float dz = centerA.z - centerB.z;
                    float len = sqrt(dx * dx + dy * dy + dz * dz);
                    if (len > 0) normal = { dx / len, dy / len, dz / len };
                    penetration = 0.1f; 
                }
            }
            else if ((typeA == Collider_Type::Sphere && typeB == Collider_Type::Box) ||
                (typeA == Collider_Type::Box && typeB == Collider_Type::Sphere))
            {
                bool aIsSphere = (typeA == Collider_Type::Sphere);

                auto* sphereCol = aIsSphere ? colA.get() : colB.get();
                auto* boxCol = aIsSphere ? colB.get() : colA.get();
                auto* boxTf = aIsSphere ? tfB.get() : tfA.get();

                XMFLOAT3 sphereCenter = aIsSphere ? centerA : centerB;
                float radius = sphereCol->GetRadius();

                auto aabb = PhysicsUtils::GetAABB(boxTf, boxCol);

                float closestX = std::max(aabb.min.x, std::min(sphereCenter.x, aabb.max.x));
                float closestY = std::max(aabb.min.y, std::min(sphereCenter.y, aabb.max.y));
                float closestZ = std::max(aabb.min.z, std::min(sphereCenter.z, aabb.max.z));

                XMFLOAT3 closestPoint = { closestX, closestY, closestZ };

                float dx = sphereCenter.x - closestPoint.x;
                float dy = sphereCenter.y - closestPoint.y;
                float dz = sphereCenter.z - closestPoint.z;
                float distSq = dx * dx + dy * dy + dz * dz;

                if (distSq < radius * radius)
                {
                    isColliding = true;
                    float dist = sqrt(distSq);
                    float sign = aIsSphere ? 1.0f : -1.0f;

                    if (dist > 0.0001f)
                    {
                        normal = { (dx / dist) * sign, (dy / dist) * sign, (dz / dist) * sign };
                        penetration = radius - dist;
                    }
                    else
                    {
                        normal = { 0.0f, 1.0f * sign, 0.0f };
                        penetration = radius;
                    }
                }
            }

            if (isColliding)
            {
                float massA = rbA->GetMass();
                float massB = rbB->GetMass();
                float totalMass = massA + massB;

                float ratioA = massB / totalMass;
                float ratioB = massA / totalMass;

                XMFLOAT3 moveA = { normal.x * penetration * ratioA, normal.y * penetration * ratioA, normal.z * penetration * ratioA };
                XMFLOAT3 moveB = { -normal.x * penetration * ratioB, -normal.y * penetration * ratioB, -normal.z * penetration * ratioB };

                posA.x += moveA.x; posA.y += moveA.y; posA.z += moveA.z;
                posB.x += moveB.x; posB.y += moveB.y; posB.z += moveB.z;

                tfA->SetPosition(posA);
                tfB->SetPosition(posB);


                XMFLOAT3 velA = rbA->GetVelocity();
                XMFLOAT3 velB = rbB->GetVelocity();
            }
        }
    }
}

void PhysicsSystem::Clear(SceneID id) 
{
    worlds.erase(id);
}