#pragma once
#include <vector>
class Component;
class MeshRendererComponent;
class CameraComponent;

class RendererManager 
{
public:
    void Register(std::weak_ptr<Component> comp);
    void RegisterCamera(std::weak_ptr<Component> cam);
    void RenderAll();

private:
    std::vector<std::weak_ptr<MeshRendererComponent>> mMeshRenderers;
    std::vector< std::weak_ptr<CameraComponent>> mCameras;
};
