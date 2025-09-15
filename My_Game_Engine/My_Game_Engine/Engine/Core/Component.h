#pragma once
#include "pch.h"

class Object;

enum Component_Type
{
    Transform,
    Mesh_Renderer,
    Camera,
    Collider,
    etc
};

class Component 
{
public:
    Component() {}

    virtual ~Component() = default;

    void SetOwner(const std::shared_ptr<Object>& owner) { mOwner = owner; }
    std::shared_ptr<Object> GetOwner() const { return mOwner.lock(); }

    virtual Component_Type GetType() const = 0;

    virtual void Update(float deltaTime) {}
    virtual void Render() {}

protected:
    std::weak_ptr<Object> mOwner;
};
