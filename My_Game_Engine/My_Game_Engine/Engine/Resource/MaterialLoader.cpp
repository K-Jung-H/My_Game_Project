#include "MaterialLoader.h"
#include "GameEngine.h"
#include "MetaIO.h"

std::shared_ptr<Material> MaterialLoader::LoadOrReuse(
    const RendererContext& ctx,
    const std::string& matFilePath,
    const std::string& uniqueName,
    const std::string& srcModelPath)
{
    ResourceSystem* rs = GameEngine::Get().GetResourceSystem();

    if (auto cached = rs->GetByPath<Material>(matFilePath))
    {
        OutputDebugStringA(("[Material] Cached material reused: " + matFilePath + "\n").c_str());
        return cached;
    }

    std::shared_ptr<Material> mat = std::make_shared<Material>();
    bool existing = std::filesystem::exists(matFilePath);

    if (existing)
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
    mat->SetPath(matFilePath);

    rs->RegisterResource(mat);

    return mat;
}