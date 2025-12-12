#pragma once
#include "Game_Resource.h"
#include "Mesh.h"
#include "Texture.h"
#include "Material.h"
#include "Model.h"
#include "Model_Avatar.h"
#include "Skeleton.h"
#include "AnimationClip.h"
#include "AvatarMask.h"
#include "MetaIO.h"
#include "TerrainResource.h"

struct LoadResult
{
    std::vector<UINT> meshIds;
    std::vector<UINT> materialIds;
    std::vector<UINT> textureIds;
	std::vector<UINT> clipIds;

    UINT modelId = Engine::INVALID_ID;
	UINT skeletonId = Engine::INVALID_ID;
	UINT avatarId = Engine::INVALID_ID;
	UINT maskID = Engine::INVALID_ID;
	UINT terrainID = Engine::INVALID_ID;
};

inline std::string MakeSubresourcePath(const std::string& containerPath, const char* kind, const std::string& nameOrIndex)
{
    return containerPath + "#" + kind + ":" + nameOrIndex;
}

struct ResourceMetaEntry
{
    std::string guid;
    std::string path;
};

struct ResourceEntry
{
    UINT id = 0;
    std::string alias; 
    const ResourceMetaEntry* metaData = nullptr;
    std::shared_ptr<Game_Resource> resource;
};

class ResourceSystem
{
public:
    void Initialize(const std::string& assetRoot);
    void Load(const std::string& path, std::string_view alias, LoadResult& result);

    void RegisterResource(const std::shared_ptr<Game_Resource>& res);


    template<typename T> std::shared_ptr<T> GetById(UINT id) const;
    template<typename T> std::shared_ptr<T> GetByGUID(const std::string& guid) const;
    template<typename T> std::shared_ptr<T> GetByPath(const std::string& path) const;
    template<typename T> std::shared_ptr<T> GetByAlias(const std::string& alias) const;
    template<typename T> std::vector<std::shared_ptr<T>> GetAllResources();
    template<typename T> std::shared_ptr<T> LoadOrReuse(const std::string& path, const std::string& alias, const RendererContext& ctx, std::function<std::shared_ptr<T>()> createCallback);
    template<typename T> std::shared_ptr<T> GetOrLoad(const std::string& guid, const std::string& path);

    const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const { return mMeshes; }
    const std::vector<std::shared_ptr<SkinnedMesh>>& GetSkinnedMeshes() const { return mSkinnedMeshes; }
    const std::vector<std::shared_ptr<Material>>& GetMaterials() const { return mMaterials; }
    const std::vector<std::shared_ptr<Texture>>& GetTextures() const { return mTextures; }
    const std::vector<std::shared_ptr<Model>>& GetModels() const { return mModels; }
    const std::vector<std::shared_ptr<Skeleton>>& GetSkeletons() const { return mSkeletons; }
    const std::vector<std::shared_ptr<Model_Avatar>>& GetAvatars() const { return mAvatars; }
    const std::vector<std::shared_ptr<AnimationClip>>& GetAnimationClips() const { return mAnimationClips; }
    const std::vector<std::shared_ptr<class AvatarMask>>& GetAvatarMasks() const { return mAvatarMasks; }
    const std::vector<std::shared_ptr<TerrainResource>>& GetTerrains() const { return mTerrains; }
    const std::unordered_map<UINT, ResourceEntry>& GetResourceMap() const { return mResources; }

    // Meta
    std::string GetOrCreateGUID(const std::string& path);
    void LoadAllMeta(const std::string& assetRoot);

    // Util
    UINT GetNextIdPreview() const { return mNextResourceID; }
    void PrintSummary() const;


private:

    std::unordered_map<UINT, ResourceEntry> mResources; // id 중심 구조

    // 빠른 Lookup용 보조 인덱스
    std::unordered_map<std::string, UINT> mGUIDToId;
    std::unordered_map<std::string, UINT> mPathToId;
    std::unordered_map<std::string, UINT> mAliasToId;

    // 타입별 캐시 (렌더링, 검색 가속)
    std::vector<std::shared_ptr<Mesh>>     mMeshes;
    std::vector<std::shared_ptr<SkinnedMesh>>     mSkinnedMeshes;
    std::vector<std::shared_ptr<Material>> mMaterials;
    std::vector<std::shared_ptr<Texture>>  mTextures;
    std::vector<std::shared_ptr<Model>>    mModels;
    std::vector<std::shared_ptr<Skeleton>> mSkeletons;
    std::vector<std::shared_ptr<Model_Avatar>> mAvatars;
    std::vector<std::shared_ptr<AnimationClip>> mAnimationClips;
    std::vector<std::shared_ptr<AvatarMask>> mAvatarMasks;
    std::vector < std::shared_ptr<TerrainResource>> mTerrains;

    // GUID 캐싱 (meta scan)
    std::deque<ResourceMetaEntry> mAllMetaData;
    std::unordered_map<std::string, ResourceMetaEntry*> mGuidToMeta;
    std::unordered_map<std::string, ResourceMetaEntry*> mPathToMeta;

    UINT mNextResourceID = 1;
};

template<typename T>
std::shared_ptr<T> ResourceSystem::GetById(UINT id) const
{
    if (auto it = mResources.find(id); it != mResources.end())
        return std::dynamic_pointer_cast<T>(it->second.resource);
    return nullptr;
}

template<typename T>
std::shared_ptr<T> ResourceSystem::GetByGUID(const std::string& guid) const
{
    if (auto it = mGUIDToId.find(guid); it != mGUIDToId.end())
        return GetById<T>(it->second);
    return nullptr;
}

template<typename T>
std::shared_ptr<T> ResourceSystem::GetByPath(const std::string& path) const
{
    if (auto it = mPathToId.find(path); it != mPathToId.end())
        return GetById<T>(it->second);
    return nullptr;
}

template<typename T>
std::shared_ptr<T> ResourceSystem::GetByAlias(const std::string& alias) const
{
    if (auto it = mAliasToId.find(alias); it != mAliasToId.end())
        return GetById<T>(it->second);
    return nullptr;
}

template<typename T>
std::vector<std::shared_ptr<T>> ResourceSystem::GetAllResources()
{
    std::vector<std::shared_ptr<T>> result;

    if constexpr (std::is_same_v<T, Mesh>)
    {
        result.reserve(mMeshes.size());
        for (const auto& res : mMeshes) result.push_back(res);
    }
    else if constexpr (std::is_same_v<T, SkinnedMesh>)
    {
        result.reserve(mSkinnedMeshes.size());
        for (const auto& res : mSkinnedMeshes) result.push_back(res);
    }
    else if constexpr (std::is_same_v<T, Material>)
    {
        result.reserve(mMaterials.size());
        for (const auto& res : mMaterials) result.push_back(res);
    }
    else if constexpr (std::is_same_v<T, Texture>)
    {
        result.reserve(mTextures.size());
        for (const auto& res : mTextures) result.push_back(res);
    }
    else if constexpr (std::is_same_v<T, Model>)
    {
        result.reserve(mModels.size());
        for (const auto& res : mModels) result.push_back(res);
    }
    else if constexpr (std::is_same_v<T, Skeleton>)
    {
        result.reserve(mSkeletons.size());
        for (const auto& res : mSkeletons) result.push_back(res);
    }
    else if constexpr (std::is_same_v<T, Model_Avatar>)
    {
        result.reserve(mAvatars.size());
        for (const auto& res : mAvatars) result.push_back(res);
    }
    else if constexpr (std::is_same_v<T, AnimationClip>)
    {
        result.reserve(mAnimationClips.size());
        for (const auto& res : mAnimationClips) result.push_back(res);
    }
    else if constexpr (std::is_same_v<T, AvatarMask>)
    {
        result.reserve(mAvatarMasks.size());
        for (const auto& res : mAvatarMasks) result.push_back(res);
    }
    else if constexpr (std::is_same_v<T, TerrainResource>)
    {
        result.reserve(mTerrains.size());
        for (const auto& res : mTerrains) result.push_back(res);
    }
    else
    {
        result.reserve(mResources.size());
        for (const auto& [id, entry] : mResources)
        {
            if (auto casted = std::dynamic_pointer_cast<T>(entry.resource))
            {
                result.push_back(casted);
            }
        }
    }

    return result;
}

template<typename T>
std::shared_ptr<T> ResourceSystem::LoadOrReuse(
    const std::string& path,
    const std::string& alias,
    const RendererContext& ctx,
    std::function<std::shared_ptr<T>()> createCallback
)
{
    if (auto cached = GetByPath<T>(path)) return cached;

    std::shared_ptr<T> resource;
    bool isNew = false;

    if (std::filesystem::exists(path))
    {
        resource = std::make_shared<T>();
        if (!resource->LoadFromFile(path, ctx))
        {
            OutputDebugStringA(("[ResourceSystem] Load failed: " + path + "\n").c_str());
            return nullptr;
        }
        OutputDebugStringA(("[ResourceSystem] Reused existing resource: " + path + "\n").c_str());
    }
    else
    {
        OutputDebugStringA(("[ResourceSystem] New resource will be created: " + path + "\n").c_str());

        resource = createCallback();
        if (!resource)
        {
            OutputDebugStringA(("[ResourceSystem] Create failed: " + path + "\n").c_str());
            return nullptr;
        }
        isNew = true;
    }

    resource->SetPath(path);
    resource->SetAlias(alias);

    if (isNew)
    {
        if (!resource->IsTemporary())
        {
            if (!resource->SaveToFile(path))
            {
                OutputDebugStringA(("[ResourceSystem] Save failed: " + path + "\n").c_str());
            }
        }
        else
        {
            OutputDebugStringA(("[ResourceSystem] Temporary resource created (Skip Save): " + path + "\n").c_str());
        }
    }

    RegisterResource(resource);

    return resource;
}

template<typename T>
std::shared_ptr<T> ResourceSystem::GetOrLoad(const std::string& guid, const std::string& path)
{
    if (!guid.empty())
    {
        if (auto res = GetByGUID<T>(guid))
            return res;
    }

    std::string loadPath = path;

    if (loadPath.empty() && !guid.empty())
    {
        if (auto it = mGuidToMeta.find(guid); it != mGuidToMeta.end())
        {
            loadPath = it->second->path;
        }
    }

    if (!loadPath.empty())
    {
        if (auto res = GetByPath<T>(loadPath))
            return res;

        if (std::filesystem::exists(loadPath))
        {
            LoadResult result;
            std::string file_name = ExtractFileName(loadPath);
            Load(loadPath, file_name, result);

            UINT targetId = Engine::INVALID_ID;

            if constexpr (std::is_same_v<T, Model>)              targetId = result.modelId;
            else if constexpr (std::is_same_v<T, Skeleton>)      targetId = result.skeletonId;
            else if constexpr (std::is_same_v<T, Model_Avatar>)  targetId = result.avatarId;
            else if constexpr (std::is_same_v<T, AvatarMask>)    targetId = result.maskID;
            else if constexpr (std::is_same_v<T, Mesh>) { if (!result.meshIds.empty()) targetId = result.meshIds[0]; }
            else if constexpr (std::is_same_v<T, Material>) { if (!result.materialIds.empty()) targetId = result.materialIds[0]; }
            else if constexpr (std::is_same_v<T, Texture>) { if (!result.textureIds.empty()) targetId = result.textureIds[0]; }
            else if constexpr (std::is_same_v<T, AnimationClip>) { if (!result.clipIds.empty()) targetId = result.clipIds[0]; }

            if (targetId != Engine::INVALID_ID)
            {
                return GetById<T>(targetId);
            }
        }
    }

    return nullptr;
}