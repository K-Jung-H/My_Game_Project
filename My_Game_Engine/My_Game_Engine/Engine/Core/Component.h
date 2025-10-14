#pragma once

class Object;

enum Component_Type
{
    Transform,
    Mesh_Renderer,
    Camera,
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

    void SetOwner(const std::shared_ptr<Object>& owner) { mOwner = owner; }
    std::shared_ptr<Object> GetOwner() const { return mOwner.lock(); }

    virtual Component_Type GetType() const = 0;

    virtual void Update() {};

    void SetActive(bool active) { Active = active; }
    bool GetActive() { return Active; }

protected:
    std::weak_ptr<Object> mOwner;
    bool Active = true;
};
