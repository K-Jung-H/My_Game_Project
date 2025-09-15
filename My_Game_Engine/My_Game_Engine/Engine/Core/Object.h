#pragma once
#include "pch.h"
#include "ComponentRegistry.h"


class Object : public std::enable_shared_from_this<Object>
{
public:
    static std::shared_ptr<Object> Create(const std::string& name);

    Object() = delete;
    ~Object();

    void SetId(UINT new_id) { object_ID = new_id; }
    UINT GetId() { return object_ID; }

    template<typename T, typename... Args>
    T& AddComponent(Args&&... args);

    template<typename T>
    T* GetComponent(Component_Type type);


    void Update(float dt);
    void Render();


private:
    explicit Object(const std::string& name) : mName(name) {}

private:
    std::string mName;
    UINT object_ID = Engine::INVALID_ID;
    std::unordered_map<Component_Type, std::shared_ptr<Component>> map_Components;
};




template<typename T>
T* Object::GetComponent(Component_Type type)
{
    auto it = map_Components.find(type);
    if (it != map_Components.end())
        return dynamic_cast<T*>(it->second.get());
    
    return nullptr;
}

template<typename T, typename... Args>
T& Object::AddComponent(Args&&... args) 
{
    Component_Type type = T::Type;
    
    if (map_Components.find(type) != map_Components.end())
        throw std::runtime_error("Component already exists for this type");

    auto comp = std::make_shared<T>(std::forward<Args>(args)...);

    T& ref = *comp;
    comp->SetOwner(shared_from_this());
    map_Components[type] = comp;
    
    ComponentRegistry::Notify(comp);
    
    return ref;
}