#pragma once
#include "Managers/ObjectManager.h"
#include "DX_Graphics/RenderData.h"

class TransformComponent;

class Object : public std::enable_shared_from_this<Object>
{
    friend class ObjectManager;

public:
    virtual rapidjson::Value ToJSON(rapidjson::Document::AllocatorType& alloc) const;
    virtual void FromJSON(const rapidjson::Value& val);

public:
    static UINT CountNodes(Object* root);
    static void DumpHierarchy(Object* root, const std::string& filename);

private:
    explicit Object(const std::string& name);


public:
    Object() = delete;
    ~Object();

    void SetId(UINT new_id) { object_ID = new_id; }
    void SetName(std::string new_name) { mName = new_name; }

    const UINT GetId() { return object_ID; }
    const std::string GetName() { return mName; } 

    std::shared_ptr<TransformComponent> GetTransform() { return transform; }
    const std::unordered_map<Component_Type, std::vector<std::shared_ptr<Component>>>& GetAllComponents() const { return map_Components; }

    template<typename T, typename... Args>
    std::shared_ptr<T> AddComponent(Args&&... args);

    template<typename T>
    void RemoveComponent();

    template<typename T>
    std::shared_ptr<T> GetComponent();

    template<typename T>
    std::vector<std::shared_ptr<T>> GetComponents();


    Object* GetParent() { return m_pParent; }
    const std::vector<Object*>& GetChildren() const { return m_pChildren; }

	void SetParent(Object* parent);
	void SetChild(Object* child);
    void SetSibling(Object* new_sibling);



    void Update_Animate(float dt);

    void UpdateTransform_All();
    void Update_Transform(const XMFLOAT4X4* parentWorld, bool parentWorldDirty);


private:
    std::string mName;
    UINT object_ID = Engine::INVALID_ID;

    std::shared_ptr<TransformComponent> transform;
    std::unordered_map<Component_Type, std::vector<std::shared_ptr<Component>>> map_Components;


    Object* m_pParent = nullptr;
    std::vector<Object*> m_pChildren;

    ObjectManager* m_pObjectManager = nullptr;

};

template<typename T, typename... Args>
std::shared_ptr<T> Object::AddComponent(Args&&... args)
{
    Component_Type type = T::Type;

    auto comp = std::make_shared<T>(std::forward<Args>(args)...);
    comp->SetOwner(this);

    map_Components[type].push_back(comp);

    if (m_pObjectManager)
        m_pObjectManager->RegisterComponent(comp);
    else
        throw std::logic_error("[Object::AddComponent] Object has no ObjectManager!\n");

    return comp;
}

template<typename T>
void Object::RemoveComponent()
{
    Component_Type type = T::Type;
    auto it = map_Components.find(type);
    if (it != map_Components.end()) 
    {
        auto& vec = it->second;
        auto vec_it = std::find_if(vec.begin(), vec.end(), [](const auto& p) 
            {
            return dynamic_cast<T*>(p.get()) != nullptr;
            });

        if (vec_it != vec.end()) 
        {
            if (m_pObjectManager) 
            {
                m_pObjectManager->UnregisterComponent(*vec_it);
            }
            vec.erase(vec_it);
        }
    }
}

template<typename T>
std::shared_ptr<T> Object::GetComponent()
{
    Component_Type type = T::Type;
    auto it = map_Components.find(type);
    if (it != map_Components.end() && !it->second.empty()) 
    {
        return std::dynamic_pointer_cast<T>(it->second.front());
    }
    return nullptr;
}

template<typename T>
std::vector<std::shared_ptr<T>> Object::GetComponents()
{
    std::vector<std::shared_ptr<T>> result;
    Component_Type type = T::Type;
    auto it = map_Components.find(type);
    if (it != map_Components.end()) 
    {
        result.reserve(it->second.size());
        for (auto& comp : it->second) 
        {
            if (auto casted = std::dynamic_pointer_cast<T>(comp)) 
            {
                result.push_back(casted);
            }
        }
    }
    return result;
}
