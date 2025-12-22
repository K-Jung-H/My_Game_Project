#pragma once

class Component;
class TransformComponent;
class ColliderComponent;
class RigidbodyComponent;
class TerrainComponent;
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

        std::vector<TerrainComponent*> terrains;
    };

    void Update(SceneID id, float dt);
    void Update_Integration(SceneID id, float dt); 
    void Update_Object_Terrain_Interact(SceneID id, float dt);
    void Update_Object_Object_Interact(SceneID id, float dt);

    void Register(SceneID id, Object* obj);
    void Unregister(SceneID id, Object* obj);

    void RegisterTerrain(SceneID id, TerrainComponent* terrain);
    void UnregisterTerrain(SceneID id, TerrainComponent* terrain);

    void Clear(SceneID id);

private:
    std::unordered_map<SceneID, World> worlds;
};