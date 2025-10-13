#include "ObjectManager.h"
#include "GameEngine.h"

ObjectManager::~ObjectManager()
{
    Clear();
}


std::shared_ptr<Object> ObjectManager::CreateObject(const std::shared_ptr<Scene>& scene, const std::string& name)
{
    UINT id = AllocateId();
    auto obj = Object::Create(scene, name);
    obj->SetId(id);
    obj->SetName(name);
    activeObjects[id] = obj;
    nameMap[name] = obj;

    return obj;
}

std::shared_ptr<Object> ObjectManager::CreateObjectWithId(const std::shared_ptr<Scene>& scene, const std::string& name, UINT id)
{
    if (activeObjects.find(id) != activeObjects.end())
    {
        OutputDebugStringA(("[ObjectManager] ID already in use: " + std::to_string(id) + "\n").c_str());
        return nullptr;
    }

    auto obj = Object::Create(scene, name);
    obj->SetId(id);

    activeObjects[id] = obj;
    nameMap[name] = obj;

    if (id >= nextID)
        nextID = id + 1;

    return obj;
}

std::shared_ptr<Object> ObjectManager::CreateFromModel(const std::shared_ptr<Scene>& scene, const std::shared_ptr<Model>& model)
{
    if (!model) return nullptr;

    auto root = Object::Create(scene, model);
    if (!root) return nullptr;


    std::queue<std::shared_ptr<Object>> q;
    q.push(root);

    while (!q.empty())
    {
        auto current = q.front();
        q.pop();

        UINT id = AllocateId();
        current->SetId(id);

        activeObjects[id] = current;
        nameMap[current->GetName()] = current;

        for (auto& child : current->GetChildren())
            q.push(child);
    }

    return root;
}


std::shared_ptr<Object> ObjectManager::GetById(UINT id)
{
    auto it = activeObjects.find(id);
    if (it != activeObjects.end())
        return it->second;
    return nullptr;
}

std::shared_ptr<Object> ObjectManager::FindByName(const std::string& name)
{
    auto it = nameMap.find(name);
    if (it != nameMap.end())
        return it->second.lock();
    return nullptr;
}


void ObjectManager::DestroyObject(UINT id)
{
    auto it = activeObjects.find(id);
    if (it == activeObjects.end())
        return;

    auto obj = it->second;
    if (obj)
    {
        for (auto& child : obj->GetChildren())
        {
            if (child)
                DestroyObject(child->GetId());
        }

        ReleaseId(id);
        nameMap.erase(obj->GetName());
        activeObjects.erase(it);
    }
}

void ObjectManager::Clear()
{
    for (auto& [id, obj] : activeObjects)
        ReleaseId(id);

    activeObjects.clear();
    nameMap.clear();

    std::queue<UINT> empty;
    std::swap(freeList, empty); 
}

UINT ObjectManager::AllocateId()
{
    if (!freeList.empty())
    {
        UINT id = freeList.front();
        freeList.pop();
        return id;
    }
    return nextID++;
}

void ObjectManager::ReleaseId(UINT id)
{
    freeList.push(id);
}
