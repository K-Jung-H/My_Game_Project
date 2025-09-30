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

void Material::FromFbxSDK(const FbxSurfaceMaterial* fbxMat)
{
    if (!fbxMat) return;

    albedoColor = XMFLOAT3(1.0f, 1.0f, 1.0f);
    roughness = 0.5f;
    metallic = 0.0f;

    // --- Diffuse Color ---
    if (fbxMat->GetClassId().Is(FbxSurfaceLambert::ClassId))
    {
        auto lambert = (FbxSurfaceLambert*)fbxMat;
        FbxDouble3 diffuse = lambert->Diffuse.Get();
        albedoColor = XMFLOAT3((float)diffuse[0], (float)diffuse[1], (float)diffuse[2]);
    }

    // --- Shininess -> Roughness  ---
    if (fbxMat->GetClassId().Is(FbxSurfacePhong::ClassId))
    {
        auto phong = (FbxSurfacePhong*)fbxMat;
        float shininess = (float)phong->Shininess.Get();
        roughness = 1.0f - std::min(shininess / 256.0f, 1.0f);
    }

    // FBX 기본 머티리얼에는 Metallic 정보가 없음
    // 필요 시, 확장 속성 찾아야 함
    metallic = 0.0f;


}