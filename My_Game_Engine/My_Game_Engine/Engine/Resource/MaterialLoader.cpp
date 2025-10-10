#include "MaterialLoader.h"
#include "ResourceRegistry.h"

std::shared_ptr<Material> MaterialLoader::LoadOrReuse(
    ResourceManager& manager,
    const RendererContext& ctx,
    const std::string& matFilePath,
    const std::string& uniqueName,
    const std::string& srcModelPath)
{
    std::shared_ptr<Material> mat = std::make_shared<Material>();

    if (std::filesystem::exists(matFilePath))
    {
        if (!mat->LoadFromFile(matFilePath, ctx))
        {
            OutputDebugStringA(("[Material] Load failed: " + matFilePath + "\n").c_str());
            return nullptr;
        }
        OutputDebugStringA(("[Material] Reused existing material: " + matFilePath + "\n").c_str());
    }
    else
    {
        OutputDebugStringA(("[Material] New material will be created: " + matFilePath + "\n").c_str());
    }

    mat->SetAlias(uniqueName);
    mat->SetId(ResourceRegistry::GenerateID());
    mat->SetPath(srcModelPath);
    manager.Add(mat);

    return mat;
}
