#pragma once
#include "pch.h"
#include "Game_Resource.h"

#define INVALID_ID 0xFFFFFFFF

class Material : public Game_Resource
{
public:
    Material();
    virtual ~Material() = default;
    virtual bool LoadFromFile(std::string_view path);
    void FromAssimp(const aiMaterial* material);


    XMFLOAT3 albedoColor;
    float roughness;
    float metallic;

    UINT diffuseTexId = INVALID_ID;
    UINT normalTexId = INVALID_ID;
    UINT roughnessTexId = INVALID_ID;
    UINT metallicTexId = INVALID_ID;
};
