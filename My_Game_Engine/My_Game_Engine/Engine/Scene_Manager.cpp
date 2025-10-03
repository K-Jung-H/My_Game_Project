#include "Scene_Manager.h"

void SceneManager::SetActiveScene(const std::shared_ptr<Scene>& scene) 
{
    mActiveScene = scene;
}

std::shared_ptr<Scene> SceneManager::GetActiveScene() const
{
    return mActiveScene.lock();
}

void SceneManager::UnloadScene(UINT id)
{
    map_Scenes.erase(id);
}


std::shared_ptr<Scene> SceneManager::CreateScene(std::string scene_alias)
{ 
    std::shared_ptr<Scene> new_Scene = std::make_shared<Scene>();

    if (mActiveScene.expired())
        mActiveScene = new_Scene;

    new_Scene->SetAlias(scene_alias);
    new_Scene->SetId(mNextSceneID++);
    new_Scene->Build();
    
    this->Add(new_Scene);

    return new_Scene;
}


void SceneManager::Add(const std::shared_ptr<Scene>& res)
{
    SceneEntry entry;
    entry.id = res->GetId();
    entry.alias = std::string(res->GetAlias());
    entry.scene = res;

    mAliasToId[entry.alias] = entry.id;
    map_Scenes[entry.id] = std::move(entry);
}

std::shared_ptr<Scene> SceneManager::GetByAlias(const std::string& alias) const
{
    if (auto it = mAliasToId.find(alias); it != mAliasToId.end())
        return map_Scenes.at(it->second).scene;
    return nullptr;
}

std::shared_ptr<Scene> SceneManager::GetById(UINT id) const
{
    if (auto it = map_Scenes.find(id); it != map_Scenes.end())
        return it->second.scene;
    return nullptr;
}
