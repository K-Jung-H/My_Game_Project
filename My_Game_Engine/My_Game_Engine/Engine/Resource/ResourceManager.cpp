#pragma once
#include "pch.h"
#include "ResourceManager.h"


void ResourceManager::Add(const std::shared_ptr<Game_Resource>& res)
{
    ResourceEntry entry;
    entry.id = res->GetId();
    entry.path = res->GetPath();
    entry.alias = std::string(res->GetAlias());
    entry.resource = res;

    mPathToId[entry.path] = entry.id;
    mAliasToId[entry.alias] = entry.id;
    map_Resources[entry.id] = std::move(entry);
}

std::shared_ptr<Game_Resource> ResourceManager::GetByPath(const std::string& path) const
{
    if (auto it = mPathToId.find(path); it != mPathToId.end())
        return map_Resources.at(it->second).resource;
    return nullptr;
}

std::shared_ptr<Game_Resource> ResourceManager::GetByAlias(const std::string& alias) const
{
    if (auto it = mAliasToId.find(alias); it != mAliasToId.end())
        return map_Resources.at(it->second).resource;
    return nullptr;
}

std::shared_ptr<Game_Resource> ResourceManager::GetById(UINT id) const
{
    if (auto it = map_Resources.find(id); it != map_Resources.end())
        return it->second.resource;
    return nullptr;
}
