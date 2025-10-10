#pragma once
#include "ResourceManager.h"
#include "Material.h"

class TextureLoader
{
public:
    static std::vector<UINT> LoadFromAssimp(ResourceManager& manager, aiMaterial* material, const std::string& basePath, std::shared_ptr<Material>& mat, const RendererContext& ctx);
    static std::vector<UINT> LoadFromFbx(ResourceManager& manager, FbxSurfaceMaterial* fbxMat, const std::string& basePath, std::shared_ptr<Material>& mat, const RendererContext& ctx);
};
