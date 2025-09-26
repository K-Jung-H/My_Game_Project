#include "pch.h"
#include "Material.h"

Material::Material() : Game_Resource()
{
    diffuseTexId = Engine::INVALID_ID;
    normalTexId = Engine::INVALID_ID;
    roughnessTexId = Engine::INVALID_ID;
    metallicTexId = Engine::INVALID_ID;

    diffuseTexSlot = UINT_MAX;
    normalTexSlot = UINT_MAX;
    roughnessTexSlot = UINT_MAX;
    metallicTexSlot = UINT_MAX;

    albedoColor = XMFLOAT3(1.0f, 1.0f, 1.0f);
    roughness = 0.5f;
    metallic = 0.0f;
}

bool Material::LoadFromFile(std::string_view path, const RendererContext& ctx)
{
    return false;
}

void Material::FromAssimp(const aiMaterial* material)
{
    aiColor4D color;
    float shininess = 0.0f;
    float metalness = 0.0f;


    if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color))
        albedoColor = XMFLOAT3(color.r, color.g, color.b);
    else
        albedoColor = XMFLOAT3(1.0f, 1.0f, 1.0f);


    if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, shininess))
        roughness = 1.0f - std::min(shininess / 256.0f, 1.0f);
    else
        roughness = 0.5f;


    if (AI_SUCCESS == material->Get(AI_MATKEY_METALLIC_FACTOR, metalness))
        metallic = metalness;
    else
        metallic = 0.0f;

}