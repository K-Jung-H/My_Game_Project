#include "ObjectManager.h"
#include "GameEngine.h"
#include "Core/Object.h"
#include "Resource/Model.h"

ObjectManager::~ObjectManager()
{
    Clear();
}

Object* ObjectManager::CreateObjectInternal(const std::string& name, UINT desired_id)
{
    UINT id = desired_id;
    if (id == 0) 
    {
        id = AllocateId();
    }
    else 
    {
        if (m_ActiveObjects.count(id)) 
        {
            OutputDebugStringA("Error: Object ID already in use.\n");
            return nullptr;
        }
        if (id >= m_NextID) 
        {
            m_NextID = id + 1;
        }
    }

    std::shared_ptr<Object> newObject = std::shared_ptr<Object>(new Object(name));

    newObject->m_pObjectManager = this;
    newObject->object_ID = id;

    std::string uniqueName = name;
    int counter = 1;
    while (m_NameToObjectMap.count(uniqueName)) 
    {
        uniqueName = name + "_" + std::to_string(counter++);
    }
    
    newObject->SetName(uniqueName);

    m_ActiveObjects[id] = newObject;
    m_NameToObjectMap[uniqueName] = newObject.get();
    m_pRootObjects.push_back(newObject.get());

    return newObject.get();
}

Object* ObjectManager::CreateObject(const std::string& name) 
{
    return CreateObjectInternal(name, 0);
}

Object* ObjectManager::CreateObjectWithId(const std::string& name, UINT id) 
{
    if (id == 0)     return nullptr;

    return CreateObjectInternal(name, id);
}

Object* ObjectManager::CreateFromModel(const std::shared_ptr<Model>& model)
{
    if (!model || !model->GetRoot()) return nullptr;

    std::vector<SkinnedMeshRendererComponent*> newSkinnedRenderers;

    std::function<Object* (const std::shared_ptr<Model::Node>&, Object*)> createNodeRecursive;

    createNodeRecursive =
        [&](const std::shared_ptr<Model::Node>& node, Object* pParent)-> Object* {
        Object* obj = CreateObject(node->name);
        if (pParent) {
            SetParent(obj, pParent);
        }
        obj->GetTransform()->SetFromMatrix(node->localTransform);

        for (auto& mesh : node->meshes)
        {
            auto skinnedMeshRes = std::dynamic_pointer_cast<SkinnedMesh>(mesh);

            if (skinnedMeshRes)
            {
                auto skinnedRenderer = obj->AddComponent<SkinnedMeshRendererComponent>();
                skinnedRenderer->SetMesh(mesh->GetId());

                newSkinnedRenderers.push_back(skinnedRenderer.get());
            }
            else
            {
                auto mr = obj->AddComponent<MeshRendererComponent>();
                mr->SetMesh(mesh->GetId());
            }
        }

        for (auto& childNode : node->children)
        {
            createNodeRecursive(childNode, obj);
        }

        return obj;
        };

    Object* rootObject = createNodeRecursive(model->GetRoot(), nullptr);

    auto animController = rootObject->AddComponent<AnimationControllerComponent>();


    for (auto* renderer : newSkinnedRenderers)
    {
        renderer->Initialize();
    }

    return rootObject;
}

void ObjectManager::DestroyObject(UINT id) 
{
    auto it = m_ActiveObjects.find(id);
    if (it != m_ActiveObjects.end()) 
    {
        DestroyObjectRecursive(it->second.get());
    }
}

void ObjectManager::DestroyObjectRecursive(Object* pObject) 
{
    if (!pObject) return;

    auto childrenCopy = pObject->GetChildren();
    for (Object* pChild : childrenCopy) 
    {
        DestroyObjectRecursive(pChild);
    }

    if (auto pParent = pObject->GetParent()) 
    {
        auto children = pParent->m_pChildren;
        children.erase(std::remove(children.begin(), children.end(), pObject), children.end());
    }
    else 
    {
        m_pRootObjects.erase(std::remove(m_pRootObjects.begin(), m_pRootObjects.end(), pObject), m_pRootObjects.end());
    }

    m_pOwnerScene->UnregisterAllComponents(pObject);

    UINT id = pObject->GetId();
    m_NameToObjectMap.erase(pObject->GetName());
    m_ActiveObjects.erase(id);
    ReleaseId(id);
}

void ObjectManager::RegisterComponent(std::shared_ptr<Component> comp) 
{
    if (m_pOwnerScene) 
    {
        m_pOwnerScene->OnComponentRegistered(comp);
    }
}

Object* ObjectManager::FindObject(UINT id) const
{
    auto it = m_ActiveObjects.find(id);
    if (it != m_ActiveObjects.end()) 
    {
        return it->second.get();
    }
    return nullptr;
}

Object* ObjectManager::FindObject(const std::string& name) const
{
    auto it = m_NameToObjectMap.find(name);
    if (it != m_NameToObjectMap.end()) 
    {
        return it->second;
    }
    return nullptr;
}


void ObjectManager::Clear()
{
    for (auto& [id, obj] : m_ActiveObjects)
        ReleaseId(id);

    m_ActiveObjects.clear();
    m_NameToObjectMap.clear();

    std::queue<UINT> empty;
    std::swap(m_FreeList, empty);
}

UINT ObjectManager::AllocateId()
{
    if (!m_FreeList.empty())
    {
        UINT id = m_FreeList.front();
        m_FreeList.pop();
        return id;
    }
    return m_NextID++;
}

void ObjectManager::ReleaseId(UINT id)
{
    m_FreeList.push(id);
}

void ObjectManager::SetObjectName(Object* pObject, const std::string& newName) 
{
    if (!pObject) return;
    m_NameToObjectMap.erase(pObject->GetName());
    pObject->SetName(newName);
    m_NameToObjectMap[newName] = pObject;
}

void ObjectManager::SetParent(Object* pChild, Object* pNewParent) 
{
    if (!pChild || pChild->m_pParent == pNewParent) 
        return;


    Object* pOldParent = pChild->m_pParent;

    if (pOldParent) 
    {
        auto& children = pOldParent->m_pChildren;
        children.erase(std::remove(children.begin(), children.end(), pChild), children.end());
    }
    else 
        m_pRootObjects.erase(std::remove(m_pRootObjects.begin(), m_pRootObjects.end(), pChild), m_pRootObjects.end());


    pChild->m_pParent = pNewParent;

    if (pNewParent) 
        pNewParent->m_pChildren.push_back(pChild);
    else 
        m_pRootObjects.push_back(pChild);
}

void ObjectManager::SetChild(Object* pParent, Object* pChild) 
{
    SetParent(pChild, pParent);
}

void ObjectManager::SetSibling(Object* pCurrentObject, Object* pNewSibling)
{
    if (!pCurrentObject || !pNewSibling) return;

    Object* pParent = pCurrentObject->GetParent();
    SetParent(pNewSibling, pParent);
}

void ObjectManager::Update_Animate_All(float dt)
{
    for (auto obj_ptr : m_pRootObjects)
    {
		obj_ptr->Update_Animate(dt);
    }
}

void ObjectManager::UpdateTransform_All()
{
    for (auto obj_ptr : m_pRootObjects)
    {
		obj_ptr->UpdateTransform_All();
    }
}