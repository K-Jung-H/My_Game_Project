#pragma once
class Scene;
class Object;
class Model;
class Component;

class ObjectManager
{
private:
    Object* CreateObjectInternal(const std::string& name, UINT id);
    void DestroyObjectRecursive(Object* pObject);
    void Clear();

public:
	explicit ObjectManager(Scene* pOwnerScene) : m_pOwnerScene(pOwnerScene) {}
    ~ObjectManager();

    Object* CreateObject(const std::string& name = "Object");
    Object* CreateObjectWithId(const std::string& name, UINT id);
    Object* CreateFromModel(const std::shared_ptr<Model>& model);
    
    void DestroyObject(UINT id);

    void RegisterComponent(std::shared_ptr<Component> comp);
    void UnregisterComponent(std::shared_ptr<Component>) {}

    UINT AllocateId();
    void ReleaseId(UINT id);

    Object* FindObject(UINT id) const;
    Object* FindObject(const std::string& name) const;


    void SetObjectName(Object* pObject, const std::string& newName);

    void SetParent(Object* pChild, Object* pNewParent);
    void SetChild(Object* pParent, Object* pChild);
    void SetSibling(Object* pCurrentObject, Object* pNewSibling);

    const std::vector<Object*>& GetRootObjects() { return m_pRootObjects; }


	void Update_Animate_All(float dt);
    void UpdateTransform_All();

private:
    Scene* m_pOwnerScene;
    UINT m_NextID = 1;

    std::queue<UINT> m_FreeList;
    
    std::unordered_map<UINT, std::shared_ptr<Object>> m_ActiveObjects;
    std::unordered_map<std::string, Object*> m_NameToObjectMap;
    std::vector<Object*> m_pRootObjects;
};