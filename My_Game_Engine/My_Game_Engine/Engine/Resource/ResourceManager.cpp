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

    if (auto mesh = std::dynamic_pointer_cast<Mesh>(res))
    {
        mMeshes.push_back(mesh);
    }
    else if (auto tex = std::dynamic_pointer_cast<Texture>(res))
    {
        mTextures.push_back(tex);
    }
    else if (auto mat = std::dynamic_pointer_cast<Material>(res))
    {
        mMaterials.push_back(mat);
    }
}
