#pragma once
#include "Component.h"
#include "ComponentRegistry.h"

class Entity {
public:
    Entity(const std::string& name);

    template<typename T, typename... Args>
    T& AddComponent(GameEngine& engine, Args&&... args);

    template<typename T>
    T* GetComponent();

    void Update(float dt);
    void Render();

private:
    std::string mName;
    std::vector<std::unique_ptr<Component>> mComponents;
};


template<typename T>
T* Entity::GetComponent() {
    for (auto& c : mComponents) {
        if (auto casted = dynamic_cast<T*>(c.get()))
            return casted;
    }
    return nullptr;
}


template<typename T, typename... Args>
T& Entity::AddComponent(GameEngine& engine, Args&&... args) {
    auto comp = std::make_unique<T>(std::forward<Args>(args)...);
    T& ref = *comp;
    comp->SetOwner(this);
    mComponents.push_back(std::move(comp));

    ComponentRegistry::Notify(engine, &ref);

    return ref;
}
