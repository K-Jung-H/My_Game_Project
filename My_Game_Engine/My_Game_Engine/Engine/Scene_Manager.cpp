#include "Scene_Manager.h"
#include "SceneArchive.h"

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

void SceneManager::SaveScene(std::shared_ptr<Scene> target_scene, std::string file_name)
{
    std::string scene_file_name = file_name.empty() ? target_scene->alias : file_name;

    std::string json_path = scene_file_name;
    std::string bin_path = scene_file_name;

    if (!HasExtension(json_path, ".json"))
        json_path += ".json";
    if (!HasExtension(bin_path, ".bin"))
        bin_path += ".bin";

    SceneArchive archive;
    archive.Save(target_scene, json_path, SceneFileFormat::JSON);
    archive.Save(target_scene, bin_path, SceneFileFormat::Binary);
}

std::shared_ptr<Scene> SceneManager::LoadScene(std::string file_name)
{
    if (file_name.empty())
        return nullptr;

    SceneArchive archive;

    bool is_json = HasExtension(file_name, ".json");
    bool is_bin = HasExtension(file_name, ".bin");

    std::shared_ptr<Scene> scene;

    if (is_json)
        scene = archive.Load(file_name, SceneFileFormat::JSON);
    else if (is_bin)
        scene = archive.Load(file_name, SceneFileFormat::Binary);
    else
    {
        scene = archive.Load(file_name + ".json", SceneFileFormat::JSON);
        if (!scene)
            scene = archive.Load(file_name + ".bin", SceneFileFormat::Binary);
    }

    return scene;
}