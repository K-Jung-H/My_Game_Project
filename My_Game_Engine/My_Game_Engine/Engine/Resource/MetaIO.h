#pragma once
#include "Game_Resource.h"

struct SubResourceMeta
{
    std::string name;
    std::string type;
    std::string guid;
};

struct FbxMeta
{
    std::string guid;
    std::string path;
    std::vector<SubResourceMeta> sub_resources;
};


namespace MetaIO
{
    std::string CreateGUID();
    void EnsureResourceGUID(const std::shared_ptr<Game_Resource>& res);

    bool SaveSimpleMeta(const std::shared_ptr<Game_Resource>& res);
    bool LoadSimpleMeta(std::shared_ptr<Game_Resource>& res);


    bool SaveFbxMeta(const FbxMeta& meta);
    bool LoadFbxMeta(FbxMeta& out, const std::string& fbxPath);
}