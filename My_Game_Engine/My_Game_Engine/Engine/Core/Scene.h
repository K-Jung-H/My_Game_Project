#pragma once
#include "DX_Graphics/RenderData.h"
#include "Managers/ObjectManager.h"

//class ObjectManager;
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


    void OnComponentRegistered(std::shared_ptr<Component> comp);
    void UnregisterAllComponents(Object* pObject);

    void RegisterRenderable(std::weak_ptr<MeshRendererComponent> comp);
    
    std::vector<Object*> GetRootObjectList() const;
    std::vector<RenderData> GetRenderable() const;
    std::vector<LightComponent*> GetLightList() const;

    void RegisterCamera(std::weak_ptr<CameraComponent> cam);
    void SetActiveCamera(const std::shared_ptr<CameraComponent>& cam) { activeCamera = cam; }

    ObjectManager* GetObjectManager() { return m_pObjectManager.get(); }
    const std::shared_ptr<CameraComponent> GetActiveCamera() { return activeCamera.lock(); }
    const std::vector<std::weak_ptr<CameraComponent>>& GetCamera_list() const { return camera_list; }
    

    virtual void Update_Inputs(float dt);
    virtual void Update_Fixed(float dt);
    virtual void Update_Scene(float dt);
    virtual void Update_Late();


protected:
    void SetId(UINT new_id) { scene_id = new_id; }
    void SetAlias(std::string a) { alias = a; }

    virtual void Build();


private:
    UINT scene_id;
    std::string alias;
    
    std::unique_ptr<ObjectManager> m_pObjectManager;

    std::vector<RenderData> renderData_list;
    std::vector<std::weak_ptr<LightComponent>> light_list;

    std::vector<std::weak_ptr<CameraComponent>> camera_list;
    std::weak_ptr<CameraComponent> activeCamera;

};