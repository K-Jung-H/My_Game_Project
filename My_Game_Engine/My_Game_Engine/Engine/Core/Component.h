#pragma once

class Object;

enum Component_Type
{
    Transform,
    Mesh_Renderer,
	Skinned_Mesh_Renderer,
    AnimationController,
    Camera,
    Terrain,
    Collider,
    Rigidbody,
    Light,
    etc
};

class Component : public std::enable_shared_from_this<Component>
{
public:
    virtual rapidjson::Value ToJSON(rapidjson::Document::AllocatorType& alloc) const { return {}; }
    virtual void FromJSON(const rapidjson::Value& val) {}

public:
    Component() {}

    virtual ~Component() = default;
    virtual Component_Type GetType() const = 0;

    virtual void WakeUp() {};
    virtual void Update() {};

    void SetOwner(Object* owner) { mOwner = owner; }
    Object* GetOwner() const { return mOwner; }

    void SetActive(bool active) { Active = active; }
    bool GetActive() { return Active; }

protected:
    Object* mOwner;
    bool Active = true;
};

class SynchronizedComponent : public Component
{
protected:
    std::mutex componentMutex;
};

class DataComponent : public Component
{
};