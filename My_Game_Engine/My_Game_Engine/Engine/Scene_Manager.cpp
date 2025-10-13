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
    if (!target_scene)
        return;

    std::filesystem::path sceneDir = EnsureSceneDirectory();

    std::string baseName = file_name.empty() ? target_scene->alias : file_name;

    baseName = std::filesystem::path(baseName).filename().string();

    std::filesystem::path json_path = sceneDir / baseName;
    std::filesystem::path bin_path = sceneDir / baseName;

    if (!HasExtension(json_path.string(), ".json"))
        json_path.replace_extension(".json");
    if (!HasExtension(bin_path.string(), ".bin"))
        bin_path.replace_extension(".bin");

    SceneArchive archive;
    archive.Save(target_scene, json_path.string(), SceneFileFormat::JSON);
    archive.Save(target_scene, bin_path.string(), SceneFileFormat::Binary);

    OutputDebugStringA(("[SceneManager] Saved: " + json_path.string() + "\n").c_str());
}

std::shared_ptr<Scene> SceneManager::LoadScene(std::string file_name)
{
    if (file_name.empty())
        return nullptr;

    std::filesystem::path sceneDir = EnsureSceneDirectory();
    std::filesystem::path path = file_name;

    if (!path.is_absolute())
        path = sceneDir / file_name;

    bool is_json = HasExtension(path.string(), ".json");
    bool is_bin = HasExtension(path.string(), ".bin");

    SceneArchive archive;
    std::shared_ptr<Scene> scene;

    if (is_json)
        scene = archive.Load(path.string(), SceneFileFormat::JSON);
    else if (is_bin)
        scene = archive.Load(path.string(), SceneFileFormat::Binary);
    else
    {
        auto json_path = path;
        json_path.replace_extension(".json");
        auto bin_path = path;
        bin_path.replace_extension(".bin");

        if (std::filesystem::exists(json_path))
            scene = archive.Load(json_path.string(), SceneFileFormat::JSON);
        else if (std::filesystem::exists(bin_path))
            scene = archive.Load(bin_path.string(), SceneFileFormat::Binary);
    }

    if (scene)
        OutputDebugStringA(("[SceneManager] Loaded: " + path.string() + "\n").c_str());
    else
        OutputDebugStringA(("[SceneManager] Load failed: " + path.string() + "\n").c_str());

    return scene;
}