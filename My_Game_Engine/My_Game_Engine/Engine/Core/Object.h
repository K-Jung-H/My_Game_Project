#pragma once
#include "ComponentRegistry.h"

class Model;

class Object : public std::enable_shared_from_this<Object>
{
public:
    static UINT CountNodes(const std::shared_ptr<Object>& root);
    static void DumpHierarchy(const std::shared_ptr<Object>& root, const std::string& filename);

public:
    static std::shared_ptr<Object> Create(const std::string& name);
    static std::shared_ptr<Object> Create(const std::shared_ptr<Model> model);

    Object() = delete;
    ~Object();

    void SetId(UINT new_id) { object_ID = new_id; }
    UINT GetId() { return object_ID; }
    std::string GetName() { return mName; }

    template<typename T, typename... Args>
    T& AddComponent(Args&&... args);

    template<typename T>
    T* GetComponent(Component_Type type);

    template<typename T>
    std::vector<T*> GetComponents(Component_Type type);

    void SetParent(std::shared_ptr<Object> parent);
    void SetChild(std::shared_ptr<Object> child);
    void SetSibling(std::shared_ptr<Object> sibling);

    std::weak_ptr<Object> GetParent();
    const std::vector<std::shared_ptr<Object>>& GetChildren();
    std::vector<std::shared_ptr<Object>> GetSiblings();


    void Update_Animate(float dt);
    void Update_Transform_All();
    void Update_Transform(const XMFLOAT4X4* parentWorld, bool parentWorldDirty);

private:
    explicit Object(const std::string& name) : mName(name) {}

private:
    std::string mName;
    UINT object_ID = Engine::INVALID_ID;
    
    std::unordered_map<Component_Type, std::vector<std::shared_ptr<Component>>> map_Components;

    std::weak_ptr<Object> parent;
    std::vector<std::shared_ptr<Object>> children;


};

template<typename T>
T* Object::GetComponent(Component_Type type)
{
    auto it = map_Components.find(type);
    if (it != map_Components.end())
        return dynamic_cast<T*>(it->second.front().get());
    
    return nullptr;
}

template<typename T>
std::vector<T*> Object::GetComponents(Component_Type type)
{
    std::vector<T*> result;
    auto it = map_Components.find(type);
    if (it != map_Components.end()) {
        for (auto& c : it->second)
            result.push_back(dynamic_cast<T*>(c.get()));
    }
    return result;
}


template<typename T, typename... Args>
T& Object::AddComponent(Args&&... args)
{
    Component_Type type = T::Type;

    auto comp = std::make_shared<T>(std::forward<Args>(args)...);
    T& ref = *comp;
    comp->SetOwner(shared_from_this());

    map_Components[type].push_back(comp);
    ComponentRegistry::Notify(comp);

    return ref;
}