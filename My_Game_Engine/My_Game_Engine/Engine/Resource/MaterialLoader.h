#pragma once
#include "ResourceManager.h"
#include "Material.h"

class MaterialLoader
{
public:
    static std::shared_ptr<Material> LoadOrReuse(ResourceManager& manager, const RendererContext& ctx, const std::string& matFilePath, const std::string& uniqueName, const std::string& srcModelPath);
};