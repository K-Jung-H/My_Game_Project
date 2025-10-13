#pragma once
#include "Material.h"

class MaterialLoader
{
public:
    static std::shared_ptr<Material> LoadOrReuse(const RendererContext& ctx, const std::string& matFilePath, const std::string& uniqueName, const std::string& srcModelPath);
};