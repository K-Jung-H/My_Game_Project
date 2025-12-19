#pragma once
#include "Components/LightComponent.h"
#include "Components/CameraComponent.h"
#include "Components/SkinnedMeshRendererComponent.h"
#include "Components/TransformComponent.h"
#include "Terrain/TerrainCommon.h"
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
    UINT materialId = 0;

    XMFLOAT4X4 World;
    D3D12_GPU_VIRTUAL_ADDRESS ObjectCBAddress;

    SkinnedMeshRendererComponent* skinnedComp = nullptr;
};

struct TerrainRenderItem
{
    TerrainPatchMesh* Mesh = nullptr;

    D3D12_GPU_VIRTUAL_ADDRESS ObjectCBAddress;
    D3D12_GPU_VIRTUAL_ADDRESS InstanceBufferAddress;

    UINT PatchVertexCount = 0;
    UINT IndexCount = 0;

    D3D12_GPU_DESCRIPTOR_HANDLE HeightMapHandle;
};