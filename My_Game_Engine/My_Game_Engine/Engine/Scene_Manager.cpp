#include "Scene_Manager.h"
#include "SceneArchive.h"

void SceneManager::SetActiveScene(const std::shared_ptr<Scene>& scene) 
{
    mActiveScene = scene;
	if(auto scene = mActiveScene.lock())
        scene->WakeUp();
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

    std::filesystem::path json_path;
    std::filesystem::path bin_path;

    if (!file_name.empty())
    {
        std::filesystem::path inputPath(file_name);

        if (inputPath.is_absolute() || inputPath.has_parent_path())
        {
            json_path = inputPath;
        }
        else
        {
            json_path = EnsureSceneDirectory() / inputPath;
        }
    }
    else
    {
        std::filesystem::path sceneDir = EnsureSceneDirectory();
        std::string name = target_scene->GetAlias();
        if (name.empty()) name = "Untitled_Scene";

        json_path = sceneDir / std::filesystem::path(name).filename();
    }

    if (!json_path.has_extension() || json_path.extension() != ".json")
    {
        json_path.replace_extension(".json");
    }

    bin_path = json_path;
    bin_path.replace_extension(".bin");

    std::filesystem::path parentDir = json_path.parent_path();
    if (!parentDir.empty() && !std::filesystem::exists(parentDir))
    {
        std::filesystem::create_directories(parentDir);
    }

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