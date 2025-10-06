#pragma once

class Component;
class MeshRendererComponent;

class RendererManager 
{
public:
    void Register(std::weak_ptr<Component> comp);

private:
    std::vector<std::weak_ptr<MeshRendererComponent>> mMeshRenderers;
};
