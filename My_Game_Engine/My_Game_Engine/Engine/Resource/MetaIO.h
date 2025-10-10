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
    struct Access
    {
        static ResourceType Type(const Game_Resource& r) { return r.resource_type; }
        static const std::string& GUID(const Game_Resource& r) { return r.GUID; }
        static const std::string& Path(const Game_Resource& r) { return r.file_path; }
        static const std::string& Alias(const Game_Resource& r) { return r.alias; }

        static void SetGUID(Game_Resource& r, const std::string& guid) { r.GUID = guid; }
        static void SetAlias(Game_Resource& r, const std::string& alias) { r.alias = alias; }
    };

    std::string CreateGUID();
    void EnsureResourceGUID(const std::shared_ptr<Game_Resource>& res);

    bool SaveSimpleMeta(const std::shared_ptr<Game_Resource>& res);
    bool LoadSimpleMeta(std::shared_ptr<Game_Resource>& res);


    bool SaveFbxMeta(const FbxMeta& meta);
    bool LoadFbxMeta(FbxMeta& out, const std::string& fbxPath);
}