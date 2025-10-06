#pragma once
#include "ComponentRegistry.h"

class Model;
class TransformComponent;
class ObjectManager;

class Object : public std::enable_shared_from_this<Object>
{
    friend class ObjectManager;

public:
    static UINT CountNodes(const std::shared_ptr<Object>& root);
    static void DumpHierarchy(const std::shared_ptr<Object>& root, const std::string& filename);

private:
    static std::shared_ptr<Object> Create(const std::string& name);
    static std::shared_ptr<Object> Create(const std::shared_ptr<Model> model);

public:
    Object() = delete;
    ~Object();

    void SetId(UINT new_id) { object_ID = new_id; }
    void SetName(std::string new_name) { mName = new_name; }

    UINT GetId() { return object_ID; }
    std::string GetName() { return mName; }
    std::shared_ptr<TransformComponent> GetTransform() { return transform; }
    std::unordered_map<Component_Type, std::vector<std::shared_ptr<Component>>> GetComponents() { return map_Components; }

    template<typename T, typename... Args>
    std::shared_ptr<T> AddComponent(Args&&... args);


    template<typename T>
    std::shared_ptr<T>  GetComponent(Component_Type type);

    template<typename T>
    T* GetComponentRaw(Component_Type type);

    template<typename T>
    std::weak_ptr<T> GetComponentWeak(Component_Type type);

    template<typename T>
    std::vector<std::shared_ptr<T>> GetComponents(Component_Type type);

    void SetParent(std::shared_ptr<Object> parent);
    void SetChild(std::shared_ptr<Object> child);
    void SetSibling(std::shared_ptr<Object> sibling);

    std::weak_ptr<Object> GetParent();
    const std::vector<std::shared_ptr<Object>>& GetChildren();
    std::vector<std::shared_ptr<Object>> GetSiblings();

    void Update_Animate(float dt);

    void UpdateTransform_All();
    void Update_Transform(const XMFLOAT4X4* parentWorld, bool parentWorldDirty);

private:
    explicit Object(const std::string& name) : mName(name) {}

private:
    std::string mName;
    UINT object_ID = Engine::INVALID_ID;
    
    std::shared_ptr<TransformComponent> transform;
    std::unordered_map<Component_Type, std::vector<std::shared_ptr<Component>>> map_Components;

    std::weak_ptr<Object> parent;
    std::vector<std::shared_ptr<Object>> children;


};


template<typename T>
std::shared_ptr<T> Object::GetComponent(Component_Type type)
{
    auto it = map_Components.find(type);
    if (it != map_Components.end())
        return std::dynamic_pointer_cast<T>(it->second.front());
    return nullptr;
}

template<typename T>
T* Object::GetComponentRaw(Component_Type type)
{
    auto it = map_Components.find(type);
    if (it != map_Components.end())
        return dynamic_cast<T*>(it->second.front().get());
    
    return nullptr;
}

template<typename T>
std::weak_ptr<T> Object::GetComponentWeak(Component_Type type)
{
    auto it = map_Components.find(type);
    if (it != map_Components.end())
        return std::dynamic_pointer_cast<T>(it->second.front());
    return {};
}

template<typename T>
std::vector<std::shared_ptr<T>> Object::GetComponents(Component_Type type)
{
    std::vector<std::shared_ptr<T>> result;
    auto it = map_Components.find(type);
    if (it != map_Components.end()) 
    {
        for (auto& c : it->second) 
        {
            if (auto casted = std::dynamic_pointer_cast<T>(c))
                result.push_back(casted);
        }
    }
    return result;
}

template<typename T, typename... Args>
std::shared_ptr<T> Object::AddComponent(Args&&... args)
{
    Component_Type type = T::Type;

    auto comp = std::make_shared<T>(std::forward<Args>(args)...);
    comp->SetOwner(shared_from_this());

    map_Components[type].push_back(comp);
    ComponentRegistry::Notify(comp);

    return comp; 
}

