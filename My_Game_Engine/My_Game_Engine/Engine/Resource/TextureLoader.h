#pragma once
#include "Material.h"

class TextureLoader
{
public:
    static std::vector<UINT> LoadFromAssimp(const RendererContext& ctx, aiMaterial* material, const std::string& basePath, std::shared_ptr<Material>& mat);
    static std::vector<UINT> LoadFromFbx(const RendererContext& ctx, FbxSurfaceMaterial* fbxMat, const std::string& basePath, std::shared_ptr<Material>& mat);
};
