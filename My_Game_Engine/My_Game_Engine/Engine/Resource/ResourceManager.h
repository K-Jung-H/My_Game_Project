#pragma once
#include "Mesh.h"
#include "Texture.h"
#include "Material.h"
#include "Model.h"

enum class FileCategory
{
    ComplexModel, 
    Texture,
    Material,
    Unknown
};

struct ResourceEntry
{
    UINT id;
    std::string path;
    std::string alias;
    std::shared_ptr<Game_Resource> resource;
};

class ResourceManager
{
public:
    void Add(const std::shared_ptr<Game_Resource>& res);

    template<typename T>
    std::shared_ptr<T> GetByGUID(const std::string& guid) const;

    template<typename T>
    std::shared_ptr<T> GetByPath(const std::string& path) const;

    template<typename T>
    std::shared_ptr<T> GetByAlias(const std::string& alias) const;

    template<typename T>
    std::shared_ptr<T> GetById(UINT id) const;

private:
    std::unordered_map<UINT, ResourceEntry> map_Resources;
    std::unordered_map<std::string, std::weak_ptr<Game_Resource>> mGuidMap;

    std::unordered_map<std::string, UINT>   mPathToId;
    std::unordered_map<std::string, UINT>   mAliasToId;

    std::vector<std::shared_ptr<Mesh>>     mMeshes;
    std::vector<std::shared_ptr<Material>> mMaterials;
    std::vector<std::shared_ptr<Texture>>  mTextures;
};

template<typename T>
std::shared_ptr<T> ResourceManager::GetByGUID(const std::string& guid) const
{
    if (auto it = mGuidMap.find(guid); it != mGuidMap.end())
    {
        if (auto res = it->second.lock())
            return std::dynamic_pointer_cast<T>(res);
    }
    return nullptr;
}

template<typename T>
std::shared_ptr<T> ResourceManager::GetByPath(const std::string& path) const
{
    if (auto it = mPathToId.find(path); it != mPathToId.end()) 
    {
        auto res = map_Resources.at(it->second).resource;
        return std::dynamic_pointer_cast<T>(res);
    }
    return nullptr;
}

template<typename T>
std::shared_ptr<T> ResourceManager::GetByAlias(const std::string& alias) const
{
    if (auto it = mAliasToId.find(alias); it != mAliasToId.end()) 
    {
        auto res = map_Resources.at(it->second).resource;
        return std::dynamic_pointer_cast<T>(res);
    }
    return nullptr;
}

template<typename T>
std::shared_ptr<T> ResourceManager::GetById(UINT id) const
{
    if (auto it = map_Resources.find(id); it != map_Resources.end()) 
    {
        auto res = it->second.resource;
        return std::dynamic_pointer_cast<T>(res);
    }
    return nullptr;
}
