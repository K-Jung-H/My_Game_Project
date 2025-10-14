#pragma once
#include "DX_Graphics/RenderData.h"


class SceneManager;
class Object;


class Scene : public std::enable_shared_from_this<Scene>
{
    friend class SceneManager;
    friend class SceneArchive;
public:
    Scene();
    virtual ~Scene();


    UINT GetId() const { return scene_id; }
    std::string GetAlias() const { return alias; }


    void RegisterObject(const std::shared_ptr<Object>& obj, bool includeComponents = false);
    void RegisterComponent(std::weak_ptr<Component> comp);
    void RegisterRenderable(std::weak_ptr<MeshRendererComponent> comp);
    std::vector<RenderData> GetRenderable() const;

    std::vector<GPULight> GetLightList() const;

    void RegisterCamera(std::weak_ptr<CameraComponent> cam);
    void SetActiveCamera(const std::shared_ptr<CameraComponent>& cam) { activeCamera = cam; }

    const std::shared_ptr<CameraComponent> GetActiveCamera() { return activeCamera.lock(); }
    const std::vector<std::weak_ptr<CameraComponent>>& GetCamera_list() const { return camera_list; }
    
    std::unordered_map<uint64_t, std::shared_ptr<Object>> GetObjectMap() { return obj_map; }
    std::vector<std::shared_ptr<Object>> GetRootObjectList() { return obj_root_list; }

    virtual void Update_Inputs(float dt);
    virtual void Update_Fixed(float dt);
    virtual void Update_Scene(float dt);
    virtual void Update_Late(float dt);

   


protected:
    void SetId(UINT new_id) { scene_id = new_id; }
    void SetAlias(std::string a) { alias = a; }

    virtual void Build();


private:
    UINT scene_id;
    std::string alias;

    std::vector<RenderData> renderData_list;

    std::vector<std::weak_ptr<CameraComponent>> camera_list;
    std::weak_ptr<CameraComponent> activeCamera;

    std::vector<std::weak_ptr<LightComponent>> light_list;


    std::unordered_map<uint64_t, std::shared_ptr<Object>> obj_map;
    std::vector<std::shared_ptr<Object>> obj_root_list;


};