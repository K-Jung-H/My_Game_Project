#pragma once
#include "pch.h"
#include "Game_Resource.h"


class Material : public Game_Resource
{
public:
    Material();
    virtual ~Material() = default;
    virtual bool LoadFromFile(std::string_view path, const RendererContext& ctx);

    void FromAssimp(const aiMaterial* material);


    XMFLOAT3 albedoColor;
    float roughness;
    float metallic;

    UINT diffuseTexId = Engine::INVALID_ID;
    UINT normalTexId = Engine::INVALID_ID;
    UINT roughnessTexId = Engine::INVALID_ID;
    UINT metallicTexId = Engine::INVALID_ID;
};
