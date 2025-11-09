#pragma once
#include "Components/LightComponent.h"
#include "Components/CameraComponent.h"
#include "Components/SkinnedMeshRendererComponent.h"
#include "Components/TransformComponent.h"
#include "Resource/Mesh.h"


struct RenderData
{
    std::weak_ptr<TransformComponent> transform;
    std::weak_ptr<MeshRendererComponent> meshRenderer;
};


struct alignas(256) ObjectCBData
{
    XMFLOAT4X4 World;

    XMFLOAT4   Albedo;
    float      Roughness;
    float      Metallic;
    float      Emissive;

    int       DiffuseTexIdx;
    int       NormalTexIdx;
    int       RoughnessTexIdx;
    int       MetallicTexIdx;
};


struct DrawItem
{
    Mesh* mesh = nullptr;
    Mesh::Submesh sub;

    UINT cbIndex = 0;
    UINT materialId = 0;

    SkinnedMeshRendererComponent* skinnedComp = nullptr;
};