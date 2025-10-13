#pragma once
#include "Core/Scene.h"

struct SceneEntry
{
    UINT id;
    std::string alias;
    std::shared_ptr<Scene> scene;
};



class SceneManager
{
public:
    static SceneManager& Get()
    {
        static SceneManager instance;
        return instance;
    }

private:
    SceneManager() = default;


public:
    ~SceneManager() = default;


public:
    void SetActiveScene(const std::shared_ptr<Scene>& scene);
    std::shared_ptr<Scene> GetActiveScene() const;
    void UnloadScene(UINT id);


    std::shared_ptr<Scene> CreateScene(std::string scene_alias);
    void Add(const std::shared_ptr<Scene>& res);

    std::shared_ptr<Scene> GetByAlias(const std::string& alias) const;
    std::shared_ptr<Scene> GetById(UINT id) const;

    void SaveScene(std::shared_ptr<Scene> target_scene, std::string file_name = "");
    std::shared_ptr<Scene> LoadScene(std::string file_name);

private:
    std::unordered_map<UINT, SceneEntry> map_Scenes;
    std::weak_ptr<Scene> mActiveScene;

    std::unordered_map<std::string, UINT>   mAliasToId;
    UINT mNextSceneID = 1;
};

static bool HasExtension(const std::string& filename, const std::string& ext)
{
    if (filename.length() < ext.length())
        return false;
    return filename.compare(filename.length() - ext.length(), ext.length(), ext) == 0;
}

static std::filesystem::path EnsureSceneDirectory()
{
    std::filesystem::path sceneDir = "Assets/Scenes";
    if (!std::filesystem::exists(sceneDir))
        std::filesystem::create_directories(sceneDir);
    return sceneDir;
}