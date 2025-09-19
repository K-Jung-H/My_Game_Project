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

private:
    std::vector<std::weak_ptr<MeshRendererComponent>> mMeshRenderers;
};
