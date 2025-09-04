#pragma once
#include <vector>
class MeshRendererComponent;
class CameraComponent;

class RendererManager {
public:
    void Register(MeshRendererComponent* comp);
    void RegisterCamera(CameraComponent* cam);
    void RenderAll();

private:
    std::vector<MeshRendererComponent*> mMeshRenderers;
    std::vector<CameraComponent*> mCameras;
};
