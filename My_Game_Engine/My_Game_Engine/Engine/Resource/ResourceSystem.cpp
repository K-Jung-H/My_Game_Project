#include "ResourceSystem.h"
#include "GameEngine.h"
#include "ModelLoader_Assimp.h"
#include "ModelLoader_FBX.h"
#include "TextureLoader.h"


void ResourceSystem::Initialize(const std::string& assetRoot)
{
    LoadAllMeta(assetRoot);
    std::cout << "[ResourceSystem] Meta cache loaded: " << mPathToGUID.size() << " entries.\n";
}

void ResourceSystem::LoadAllMeta(const std::string& assetRoot)
{
    namespace fs = std::filesystem;
    for (auto& p : fs::recursive_directory_iterator(assetRoot))
    {
        if (p.path().extension() == ".meta")
        {
            std::ifstream ifs(p.path());
            if (!ifs.is_open())
                continue;

            std::string json((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            Document doc;
            if (doc.Parse(json.c_str()).HasParseError())
                continue;

            if (doc.HasMember("path") && doc.HasMember("guid"))
            {
                std::string filePath = doc["path"].GetString();
                std::string guid = doc["guid"].GetString();
                mPathToGUID[filePath] = guid;
            }
        }
    }
}

std::string ResourceSystem::GetOrCreateGUID(const std::string& path)
{
    if (auto it = mPathToGUID.find(path); it != mPathToGUID.end())
        return it->second;

    std::string guid = MetaIO::CreateGUID();
    mPathToGUID[path] = guid;
    return guid;
}

void ResourceSystem::RegisterResource(const std::shared_ptr<Game_Resource>& res)
{
    if (!res) return;

    ResourceEntry entry;
    entry.id = mNextResourceID++;
    entry.path = res->GetPath();
    entry.alias = res->GetAlias();
    entry.resource = res;

    if (res->GetGUID().empty())
        entry.guid = MetaIO::CreateGUID(entry.path, entry.alias);
    else
        entry.guid = res->GetGUID();

    res->SetId(entry.id);
    res->SetGUID(entry.guid);

    mResources[entry.id] = entry;
    mGUIDToId[entry.guid] = entry.id;
    mPathToId[entry.path] = entry.id;
    if (!entry.alias.empty()) mAliasToId[entry.alias] = entry.id;

    // type cache
    switch (res->Get_Type())
    {
    case ResourceType::Mesh:     mMeshes.push_back(std::dynamic_pointer_cast<Mesh>(res)); break;
    case ResourceType::Material: mMaterials.push_back(std::dynamic_pointer_cast<Material>(res)); break;
    case ResourceType::Texture:  mTextures.push_back(std::dynamic_pointer_cast<Texture>(res)); break;
    case ResourceType::Model:    mModels.push_back(std::dynamic_pointer_cast<Model>(res)); break;
    case ResourceType::Skeleton: mSkeletons.push_back(std::dynamic_pointer_cast<Skeleton>(res)); break;
    case ResourceType::ModelAvatar: mAvatars.push_back(std::dynamic_pointer_cast<Model_Avatar>(res)); break;
    case ResourceType::AnimationClip: mAnimationClips.push_back(std::dynamic_pointer_cast<AnimationClip>(res)); break;

    default: break;
    }

    if (res->GetPath().find('#') == std::string::npos)
    {
        MetaIO::SaveSimpleMeta(res);
    }


}

void ResourceSystem::Load(const std::string& path, std::string_view alias, LoadResult& result)
{
    const RendererContext& ctx = GameEngine::Get().Get_UploadContext();
    FileCategory category = DetectFileCategory(path);

    // 캐시 검사: 이미 로드된 리소스가 있으면 결과에 ID만 기록
    if (auto it = mPathToId.find(path); it != mPathToId.end())
    {
        if (auto res = GetById<Game_Resource>(it->second))
        {
            OutputDebugStringA(("[ResourceSystem] Cached resource hit: " + path + "\n").c_str());

            switch (res->Get_Type())
            {
            case ResourceType::Model:    result.modelId = res->GetId(); break;
            case ResourceType::Material: result.materialIds.push_back(res->GetId()); break;
            case ResourceType::Texture:  result.textureIds.push_back(res->GetId()); break;
            default: break;
            }
            return;
        }
    }

    // -------------------------------
    // 리소스 타입별 로드
    // -------------------------------
    switch (category)
    {
    case FileCategory::FBX:
    {
        ModelLoader_FBX fbxLoader;
        if (fbxLoader.Load(path, alias, result))
        {
            OutputDebugStringA(("[FBX SDK] Loaded model: " + path + "\n").c_str());
            return;
        }

        ModelLoader_Assimp assimpLoader;
        if (assimpLoader.Load(path, alias, result))
        {
            OutputDebugStringA(("[Assimp] Loaded FBX: " + path + "\n").c_str());
            return;
        }

        OutputDebugStringA(("[ResourceSystem] Failed to load FBX file: " + path + "\n").c_str());
        break;
    }

    case FileCategory::ComplexModel:
    {
        ModelLoader_Assimp assimpLoader;
        if (assimpLoader.Load(path, alias, result))
        {
            OutputDebugStringA(("[Assimp] Loaded model: " + path + "\n").c_str());
            return;
        }

        OutputDebugStringA(("[ResourceSystem] Failed to load complex model: " + path + "\n").c_str());
        break;
    }

    // ----------------------------------------------
    // 머티리얼 파일
    // ----------------------------------------------
    case FileCategory::Material:
    {
        auto mat = LoadOrReuse<Material>(path, std::string(alias), ctx, 
            [&]() -> std::shared_ptr<Material> {
                return std::make_shared<Material>();
            }
        );

        if (mat)
        {
            result.materialIds.push_back(mat->GetId());
        }
        break;
    }

    // ----------------------------------------------
    // 텍스처 파일
    // ----------------------------------------------
    case FileCategory::Texture:
    {
        auto tex = std::make_shared<Texture>();
        if (tex->LoadFromFile(path, ctx))
        {
            tex->SetAlias(std::string(alias));
            RegisterResource(tex);
            result.textureIds.push_back(tex->GetId());
        }
        else
        {
            OutputDebugStringA(("[ResourceSystem] Texture load failed: " + path + "\n").c_str());
        }
        break;
    }

    default:
        OutputDebugStringA(("[ResourceSystem] Unknown resource type: " + path + "\n").c_str());
        break;
    }
}


void ResourceSystem::PrintSummary() const
{
    std::cout << "\n===== ResourceSystem Summary =====\n";
    std::cout << "Total Resources: " << mResources.size() << "\n";
    std::cout << "Meshes: " << mMeshes.size()
        << "  Materials: " << mMaterials.size()
        << "  Textures: " << mTextures.size()
        << "  Models: " << mModels.size() << "\n";

    for (auto& [id, entry] : mResources)
    {
        std::cout << "  [" << id << "] " << entry.path
            << " | GUID: " << entry.guid
            << " | Alias: " << entry.alias << "\n";
    }
}