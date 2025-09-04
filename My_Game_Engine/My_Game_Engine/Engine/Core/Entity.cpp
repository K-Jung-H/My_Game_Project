#include "pch.h"
#include "Entity.h"
#include "ComponentRegistry.h"

Entity::Entity(const std::string& name) : mName(name) {}

void Entity::Update(float dt) {
    for (auto& c : mComponents)
        c->Update(dt);
}

void Entity::Render() {
    for (auto& c : mComponents)
        c->Render();
}
