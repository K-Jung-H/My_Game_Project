#pragma once
#include "pch.h"
#include "Object.h"

class SceneManager;

class Scene
{
    friend class SceneManager;

public:
    Scene();
    virtual ~Scene();


    UINT GetId() const { return scene_id; }
    std::string_view GetAlias() const { return alias; }

    void RegisterRenderable(std::weak_ptr<MeshRendererComponent> comp);
    std::vector<std::shared_ptr<MeshRendererComponent>> GetRenderable() const;

    void RegisterCamera(std::weak_ptr<CameraComponent> cam);

    const std::shared_ptr<CameraComponent> GetActiveCamera() { return activeCamera.lock(); }
    const std::vector<std::weak_ptr<CameraComponent>>& GetCamera_list() const { return camera_list; }
    void SetActiveCamera(std::weak_ptr<CameraComponent> cam) { activeCamera = cam; }

    void Check_Inputs();
    void Fixed_Update(float ElapsedTime); // For Physics
    virtual void Update(float ElapsedTime); // Game Logic
    virtual void Render();


protected:
    void SetId(UINT new_id) { scene_id = new_id; }
    void SetAlias(std::string_view a) { alias = a; }

    virtual void Build();


private:
    UINT scene_id;
    std::string alias;

    std::vector<std::weak_ptr<MeshRendererComponent>> renderable_list;
    std::vector<std::weak_ptr<CameraComponent>> camera_list;
    std::weak_ptr<CameraComponent> activeCamera;
    std::vector<std::shared_ptr<Object>> obj_list;
};