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

    void Check_Inputs();
    void Fixed_Update(float ElapsedTime);
    void Update(float ElapsedTime);
    void Render();

public:
    void SetActiveScene(const std::shared_ptr<Scene>& scene);
    std::shared_ptr<Scene> GetActiveScene() const;
    void UnloadScene(UINT id);


    std::shared_ptr<Scene> CreateScene(std::string scene_alias);
    void Add(const std::shared_ptr<Scene>& res);

    std::shared_ptr<Scene> GetByAlias(const std::string& alias) const;
    std::shared_ptr<Scene> GetById(UINT id) const;

private:
    std::unordered_map<UINT, SceneEntry> map_Scenes;
    std::weak_ptr<Scene> mActiveScene;

    std::unordered_map<std::string, UINT>   mAliasToId;
    UINT mNextSceneID = 1;
};