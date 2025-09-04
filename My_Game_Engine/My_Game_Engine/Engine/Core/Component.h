#pragma once
#include "pch.h"
#include "GameEngine.h"

class Entity;

class Component 
{
public:
    virtual ~Component() = default;

    void SetOwner(Entity* owner) { mOwner = owner; }
    Entity* GetOwner() const { return mOwner; }

    virtual void Update(float deltaTime) {}
    virtual void Render() {}

protected:
    Entity* mOwner = nullptr;
};
