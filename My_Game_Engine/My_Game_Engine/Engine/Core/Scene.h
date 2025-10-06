#pragma once
#include "Object.h"
#include "DX_Graphics/RenderData.h"


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
    std::vector<RenderData> GetRenderable() const;

    void RegisterCamera(std::weak_ptr<CameraComponent> cam);

    const std::shared_ptr<CameraComponent> GetActiveCamera() { return activeCamera.lock(); }
    const std::vector<std::weak_ptr<CameraComponent>>& GetCamera_list() const { return camera_list; }
    void SetActiveCamera(const std::shared_ptr<CameraComponent>& cam) { activeCamera = cam; }

    virtual void Update_Inputs(float dt);
    virtual void Update_Fixed(float dt);
    virtual void Update_Scene(float dt);
    virtual void Update_Late(float dt);

    virtual void Render() {}


protected:
    void SetId(UINT new_id) { scene_id = new_id; }
    void SetAlias(std::string_view a) { alias = a; }

    virtual void Build();


private:
    UINT scene_id;
    std::string alias;

    std::vector<RenderData> renderData_list;
    std::vector<std::weak_ptr<CameraComponent>> camera_list;
    std::weak_ptr<CameraComponent> activeCamera;

    std::unordered_map<uint64_t, std::shared_ptr<Object>> obj_map;
    std::vector<std::shared_ptr<Object>> obj_root_list;
};