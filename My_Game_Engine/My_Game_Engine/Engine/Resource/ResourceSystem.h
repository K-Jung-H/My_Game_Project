#pragma once
#include "Game_Resource.h"
#include "Mesh.h"
#include "Texture.h"
#include "Material.h"
#include "Model.h"
#include "Model_Avatar.h"
#include "Skeleton.h"
#include "AnimationClip.h"
#include "MetaIO.h"

struct LoadResult
{
    std::vector<UINT> meshIds;
    std::vector<UINT> materialIds;
    std::vector<UINT> textureIds;
    UINT modelId = Engine::INVALID_ID;
};

inline std::string MakeSubresourcePath(const std::string& containerPath, const char* kind, const std::string& nameOrIndex)
{
    return containerPath + "#" + kind + ":" + nameOrIndex;
}

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
    template<typename T> std::shared_ptr<T> LoadOrReuse(const std::string& path, const std::string& alias, const RendererContext& ctx, std::function<std::shared_ptr<T>()> createCallback);

    // Meta
    std::string GetOrCreateGUID(const std::string& path);
    void LoadAllMeta(const std::string& assetRoot);

    // Util
    UINT GetNextIdPreview() const { return mNextResourceID; }
    void PrintSummary() const;


private:
    struct ResourceEntry
    {
        UINT id = 0;
        std::string guid;
        std::string path;
        std::string alias;
        std::shared_ptr<Game_Resource> resource;
    };

    std::unordered_map<UINT, ResourceEntry> mResources; // id 중심 구조

    // 빠른 Lookup용 보조 인덱스
    std::unordered_map<std::string, UINT> mGUIDToId;
    std::unordered_map<std::string, UINT> mPathToId;
    std::unordered_map<std::string, UINT> mAliasToId;

    // 타입별 캐시 (렌더링, 검색 가속)
    std::vector<std::shared_ptr<Mesh>>     mMeshes;
    std::vector<std::shared_ptr<Material>> mMaterials;
    std::vector<std::shared_ptr<Texture>>  mTextures;
    std::vector<std::shared_ptr<Model>>    mModels;
    std::vector<std::shared_ptr<Skeleton>> mSkeletons;
    std::vector<std::shared_ptr<Model_Avatar>> mAvatars;
    std::vector<std::shared_ptr<AnimationClip>> mAnimationClips;

    // GUID 캐싱 (meta scan)
    std::unordered_map<std::string, std::string> mPathToGUID;


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
std::shared_ptr<T> ResourceSystem::LoadOrReuse(
    const std::string& path,
    const std::string& alias,
    const RendererContext& ctx,
    std::function<std::shared_ptr<T>()> createCallback
)
{
    if (auto cached = GetByPath<T>(path))
    {
        return cached;
    }

    std::shared_ptr<T> resource;
    bool existing = std::filesystem::exists(path);

    if (existing)
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

        resource->SetPath(path);
        resource->SetAlias(alias);

        if (!resource->SaveToFile(path))
        {
            OutputDebugStringA(("[ResourceSystem] Save failed: " + path + "\n").c_str());
        }
    }

    resource->SetAlias(alias);
    resource->SetPath(path);
    RegisterResource(resource);

    return resource;
}