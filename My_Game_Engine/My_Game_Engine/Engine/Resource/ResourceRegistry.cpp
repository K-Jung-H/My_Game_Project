#include "ResourceRegistry.h"
#include "ModelLoader_Assimp.h"
#include "ModelLoader_FBX.h"
#include "DXMathUtils.h"
#include "MetaIO.h"

LoadResult ResourceRegistry::Load(ResourceManager& manager, const std::string& path, std::string_view alias, const RendererContext& ctx)
{
    LoadResult result;

    {
        ModelLoader_Assimp assimpLoader;
        if (assimpLoader.Load(manager, path, alias, ctx, result))
            return result;
    }

    {
        ModelLoader_FBX fbxLoader;
        if (fbxLoader.Load(manager, path, alias, ctx, result))
            return result;
    }

    // --- ½ÇÆÐ ½Ã ---
    OutputDebugStringA(("[Loader] Failed to load resource: " + path + "\n").c_str());
    return result;
}
