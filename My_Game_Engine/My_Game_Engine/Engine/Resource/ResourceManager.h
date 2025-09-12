#pragma once
#include "pch.h"
#include "Mesh.h"
#include "Texture.h"
#include "Material.h"
#include "Game_Resource.h"

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

    std::shared_ptr<Game_Resource> GetByPath(const std::string& path) const;
    std::shared_ptr<Game_Resource> GetByAlias(const std::string& alias) const;
    std::shared_ptr<Game_Resource> GetById(UINT id) const;

private:
    std::unordered_map<UINT, ResourceEntry> map_Resources;
    std::unordered_map<std::string, UINT>   mPathToId;
    std::unordered_map<std::string, UINT>   mAliasToId;

    std::vector<std::shared_ptr<Mesh>>     mMeshes;
    std::vector<std::shared_ptr<Material>> mMaterials;
    std::vector<std::shared_ptr<Texture>>  mTextures;
};
