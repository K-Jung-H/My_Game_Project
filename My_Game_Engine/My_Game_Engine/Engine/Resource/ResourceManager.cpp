#pragma once
#include "ResourceManager.h"


void ResourceManager::Add(const std::shared_ptr<Game_Resource>& res)
{
    ResourceEntry entry;
    entry.id = res->GetId();
    entry.path = res->GetPath();
    entry.alias = std::string(res->GetAlias());
    entry.resource = res;

    if (res->GetGUID().empty())
    {
        std::string metaPath = entry.path + ".meta";

        if (std::filesystem::exists(metaPath))
        {
            std::ifstream file(metaPath);
            std::string guid;
            file >> guid;
            res->SetGUID(guid);
        }
        else
        {
            std::string guid = GenerateGUID();
            res->SetGUID(guid);

            std::ofstream file(metaPath, std::ios::trunc);
            file << guid;
        }
    }

    mGuidMap[res->GetGUID()] = res;

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
