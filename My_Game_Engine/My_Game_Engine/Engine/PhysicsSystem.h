#pragma once

class Component;
class TransformComponent;
class ColliderComponent;
class RigidbodyComponent;
class Object;


using SceneID = UINT;


class PhysicsSystem 
{
public:
    struct Entry 
    {
        std::weak_ptr<TransformComponent> tf;
        std::weak_ptr<RigidbodyComponent> rb;
        std::weak_ptr<ColliderComponent> col;
    };

    struct World 
    {
        std::vector<Entry> dynamics; // Transform + Rigidbody + Collider
        std::vector<Entry> statics; // Transform + Collider
    };

    void Update(SceneID id, float dt);


    void Register(SceneID id, Object* obj);
    void Unregister(SceneID id, Object* obj);
    void Clear(SceneID id);

private:
    std::unordered_map<SceneID, World> worlds;
};