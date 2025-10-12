#pragma once
#include "Game_Resource.h"


class Material : public Game_Resource
{
public:
    Material();
    virtual ~Material() = default;
    virtual bool LoadFromFile(std::string path, const RendererContext& ctx);
    bool SaveToFile(const std::string& outputPath) const;

    void FromAssimp(const aiMaterial* material);
    void FromFbxSDK(const FbxSurfaceMaterial* material);

    void Bind(ComPtr<ID3D12GraphicsCommandList> cmdList) {}

    XMFLOAT3 albedoColor;
    float roughness;
    float metallic;

    UINT diffuseTexId = Engine::INVALID_ID;
    UINT normalTexId = Engine::INVALID_ID;
    UINT roughnessTexId = Engine::INVALID_ID;
    UINT metallicTexId = Engine::INVALID_ID;

    UINT diffuseTexSlot = UINT_MAX;
    UINT normalTexSlot = UINT_MAX;
    UINT roughnessTexSlot = UINT_MAX;
    UINT metallicTexSlot = UINT_MAX;

    std::string shaderName; // 임시 구조
};
