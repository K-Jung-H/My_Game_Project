#include "ResourceSystem.h"
#include "GameEngine.h"
#include "ModelLoader_Assimp.h"
#include "ModelLoader_FBX.h"
#include "TextureLoader.h"


void ResourceSystem::Initialize(const std::string& assetRoot)
{
    LoadAllMeta(assetRoot);
    std::string output = "[ResourceSystem] Meta cache loaded: " + std::to_string(mAllMetaData.size()) + " entries.\n";
	OutputDebugStringA(output.c_str());
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
            rapidjson::Document doc;

            if (doc.Parse(json.c_str()).HasParseError())
                continue;

            if (doc.HasMember("path") && doc.HasMember("guid"))
            {
                std::string filePath = doc["path"].GetString();
                std::string rootGuid = doc["guid"].GetString();

                ResourceMetaEntry& entry = mAllMetaData.emplace_back();
                entry.path = filePath;
                entry.guid = rootGuid;

                ResourceMetaEntry* entryPtr = &entry;

                mPathToMeta[filePath] = entryPtr;
                mGuidToMeta[rootGuid] = entryPtr;

                if (doc.HasMember("sub_resources") && doc["sub_resources"].IsArray())
                {
                    const auto& subArray = doc["sub_resources"].GetArray();
                    for (rapidjson::SizeType i = 0; i < subArray.Size(); ++i)
                    {
                        const auto& subItem = subArray[i];
                        if (subItem.IsObject() && subItem.HasMember("guid") && subItem["guid"].IsString())
                        {
                            std::string subGuid = subItem["guid"].GetString();

                            mGuidToMeta[subGuid] = entryPtr;
                        }
                    }
                }
            }
        }
    }
}

std::string ResourceSystem::GetOrCreateGUID(const std::string& path)
{
    if (auto it = mPathToMeta.find(path); it != mPathToMeta.end())
    {
        return it->second->guid;
    }

    std::string guid = MetaIO::CreateGUID(path, "");

    ResourceMetaEntry& entry = mAllMetaData.emplace_back();
    entry.path = path;
    entry.guid = guid;

    ResourceMetaEntry* entryPtr = &entry;
    mPathToMeta[path] = entryPtr;
    mGuidToMeta[guid] = entryPtr;

    return guid;
}

void ResourceSystem::RegisterResource(const std::shared_ptr<Game_Resource>& res)
{
    if (!res) return;

    if (res->IsTemporary())
    {
        res->SetId(Engine::INVALID_ID);
        return;
    }

    std::string baseAlias = res->GetAlias();
    if (!baseAlias.empty())
    {
        std::string uniqueAlias = baseAlias;
        int count = 1;

        while (mAliasToId.find(uniqueAlias) != mAliasToId.end())
        {
            uniqueAlias = baseAlias + "_" + std::to_string(count++);
        }

        if (uniqueAlias != baseAlias)
        {
            res->SetAlias(uniqueAlias);
        }
    }

    std::string resourcePath = res->GetPath();
    std::string resourceGUID = res->GetGUID();

    if (resourceGUID.empty())
    {
        auto it = mPathToMeta.find(resourcePath);
        if (it != mPathToMeta.end())
        {
            resourceGUID = it->second->guid;
        }
        else
        {
            resourceGUID = MetaIO::CreateGUID(resourcePath, res->GetAlias());
        }

        res->SetGUID(resourceGUID);
    }

    ResourceMetaEntry* metaPtr = nullptr;
    auto metaIt = mGuidToMeta.find(resourceGUID);

    if (metaIt != mGuidToMeta.end())
    {
        metaPtr = metaIt->second;
    }
    else
    {
        ResourceMetaEntry& newMeta = mAllMetaData.emplace_back();
        newMeta.guid = resourceGUID;
        newMeta.path = resourcePath;

        metaPtr = &newMeta;

        mGuidToMeta[resourceGUID] = metaPtr;
        mPathToMeta[resourcePath] = metaPtr;
    }

    ResourceEntry entry;
    entry.id = mNextResourceID++;
    entry.alias = res->GetAlias();
    entry.metaData = metaPtr;
    entry.resource = res;

    res->SetId(entry.id);

    mResources[entry.id] = entry;
    mGUIDToId[resourceGUID] = entry.id;
    mPathToId[resourcePath] = entry.id;

    if (!entry.alias.empty())
    {
        mAliasToId[entry.alias] = entry.id;
    }

    switch (res->Get_Type())
    {
    case ResourceType::Mesh:
    {
        auto mesh = std::dynamic_pointer_cast<Mesh>(res);
        if (mesh)
        {
            mMeshes.push_back(mesh);
            auto skinnedmesh = std::dynamic_pointer_cast<SkinnedMesh>(res);
            if (skinnedmesh)
                mSkinnedMeshes.push_back(skinnedmesh);
        }
    }
    break;
    case ResourceType::Material:      mMaterials.push_back(std::dynamic_pointer_cast<Material>(res)); break;
    case ResourceType::Texture:       mTextures.push_back(std::dynamic_pointer_cast<Texture>(res)); break;
    case ResourceType::Model:         mModels.push_back(std::dynamic_pointer_cast<Model>(res)); break;
    case ResourceType::Skeleton:      mSkeletons.push_back(std::dynamic_pointer_cast<Skeleton>(res)); break;
    case ResourceType::ModelAvatar:   mAvatars.push_back(std::dynamic_pointer_cast<Model_Avatar>(res)); break;
    case ResourceType::AnimationClip: mAnimationClips.push_back(std::dynamic_pointer_cast<AnimationClip>(res)); break;
    case ResourceType::AvatarMask:    mAvatarMasks.push_back(std::dynamic_pointer_cast<AvatarMask>(res)); break;
    default: break;
    }

    if (resourcePath.find('#') == std::string::npos)
    {
        MetaIO::SaveSimpleMeta(res);
    }
}

void ResourceSystem::Load(const std::string& path, std::string_view alias, LoadResult& result)
{
    std::string normalized_path = NormalizeFilePath(path);
    FileCategory category = DetectFileCategory(normalized_path);

    if (auto it = mPathToId.find(path); it != mPathToId.end())
    {
        if (auto res = GetById<Game_Resource>(it->second))
        {
            OutputDebugStringA(("[ResourceSystem] Cached resource hit: " + normalized_path + "\n").c_str());

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
    const RendererContext& ctx = GameEngine::Get().Get_UploadContext();
    auto renderer = GameEngine::Get().GetRenderer();
    bool bManagedByExternal = renderer->IsUploadOpen(); 

    if (!bManagedByExternal)
        renderer->BeginUpload();

    switch (category)
    {
    case FileCategory::FBX:
    {
        ModelLoader_FBX fbxLoader;
        if (fbxLoader.Load(normalized_path, alias, result))
        {
            OutputDebugStringA(("[FBX SDK] Loaded model: " + normalized_path + "\n").c_str());
            return;
        }

        ModelLoader_Assimp assimpLoader;
        if (assimpLoader.Load(normalized_path, alias, result))
        {
            OutputDebugStringA(("[Assimp] Loaded FBX: " + normalized_path + "\n").c_str());
            return;
        }

        OutputDebugStringA(("[ResourceSystem] Failed to load FBX file: " + normalized_path + "\n").c_str());
        break;
    }

    case FileCategory::ComplexModel:
    {
        ModelLoader_Assimp assimpLoader;
        if (assimpLoader.Load(normalized_path, alias, result))
        {
            OutputDebugStringA(("[Assimp] Loaded model: " + normalized_path + "\n").c_str());
            return;
        }

        OutputDebugStringA(("[ResourceSystem] Failed to load complex model: " + normalized_path + "\n").c_str());
        break;
    }

    // ----------------------------------------------
    // 머티리얼 파일
    // ----------------------------------------------
    case FileCategory::Material:
    {
        auto mat = LoadOrReuse<Material>(normalized_path, std::string(alias), ctx,
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
        if (tex->LoadFromFile(normalized_path, ctx))
        {
            tex->SetAlias(std::string(alias));
            RegisterResource(tex);
            result.textureIds.push_back(tex->GetId());
        }
        else
        {
            OutputDebugStringA(("[ResourceSystem] Texture load failed: " + normalized_path + "\n").c_str());
        }
        break;
    }
    
    case FileCategory::Clip:
    {
        auto clip = LoadOrReuse<AnimationClip>(normalized_path, std::string(alias), ctx,
            [&]() -> std::shared_ptr<AnimationClip> {
                return std::make_shared<AnimationClip>();
            }
        );
        if (clip)
            result.clipIds.push_back(clip->GetId());
        break;
    }

    case FileCategory::Skeleton:
    {
        auto skel = LoadOrReuse<Skeleton>(normalized_path, std::string(alias), ctx,
            [&]() -> std::shared_ptr<Skeleton> {
                return std::make_shared<Skeleton>();
            }
        );
        if (skel)
            result.skeletonId = skel->GetId();
        break;
	}

    case FileCategory::Model_Avatar:
    {
        auto avatar = LoadOrReuse<Model_Avatar>(normalized_path, std::string(alias), ctx,
            [&]() -> std::shared_ptr<Model_Avatar> {
                return std::make_shared<Model_Avatar>();
            }
        );
        if (avatar)
            result.avatarId = avatar->GetId();
        break;
	}

    case FileCategory::AvatarMask:
    {
        auto mask = LoadOrReuse<AvatarMask>(normalized_path, std::string(alias), ctx,
            [&]() -> std::shared_ptr<AvatarMask> {
                return std::make_shared<AvatarMask>();
            }
        );

        if (mask)
            result.maskID = mask->GetId();

        break;
    }

	case FileCategory::RawData:
    {
        auto terrain = LoadOrReuse<TerrainResource>(normalized_path, std::string(alias), ctx,
            [&]() -> std::shared_ptr<TerrainResource> {
                return std::make_shared<TerrainResource>();
            }
		);

        if (terrain)
			result.terrainID = terrain->GetId();
        break;
    }

    default:
        OutputDebugStringA(("[ResourceSystem] Unknown resource type: " + normalized_path + "\n").c_str());
        break;
    }

    if (!bManagedByExternal)
		renderer->EndUpload();
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
        std::cout << "  [" << id << "] " << entry.metaData->path
            << " | GUID: " << entry.metaData->guid
            << " | Alias: " << entry.alias << "\n";
    }
}