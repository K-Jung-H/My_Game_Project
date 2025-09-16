#pragma once
#include <vector>
#include "../Components/MeshRendererComponent.h"
#include "../Components/CameraComponent.h"

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
