#include "MetaIO.h"
#include "Game_Resource.h"





std::string MetaIO::CreateGUID()
{
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;

    uint64_t high = dis(gen);
    uint64_t low = dis(gen);

    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << high
        << std::setw(16) << std::setfill('0') << low;
    return ss.str();
}

void MetaIO::EnsureResourceGUID(const std::shared_ptr<Game_Resource>& res)
{
    if (!res) return;
    if (!Access::GUID(*res).empty()) return;

    std::string metaPath = Access::Path(*res) + ".meta";

    if (std::filesystem::exists(metaPath))
    {
        std::ifstream ifs(metaPath);
        if (ifs.is_open())
        {
            std::string json((std::istreambuf_iterator<char>(ifs)),
                std::istreambuf_iterator<char>());

            Document doc;
            if (!doc.Parse(json.c_str()).HasParseError() && doc.HasMember("guid"))
            {
                Access::SetGUID(*res, doc["guid"].GetString());
                return;
            }
        }
    }

    Access::SetGUID(*res, CreateGUID());
}


bool MetaIO::SaveSimpleMeta(const std::shared_ptr<Game_Resource>& res)
{
    if (!res) return false;

    Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("guid", Value(Access::GUID(*res).c_str(), alloc), alloc);

    std::string typeStr;
    switch (Access::Type(*res))
    {
    case ResourceType::Mesh: typeStr = "Mesh"; break;
    case ResourceType::Material: typeStr = "Material"; break;
    case ResourceType::Texture: typeStr = "Texture"; break;
    default: typeStr = "etc"; break;
    }
    doc.AddMember("type", Value(typeStr.c_str(), alloc), alloc);
    doc.AddMember("path", Value(Access::Path(*res).c_str(), alloc), alloc);
    doc.AddMember("alias", Value(Access::Alias(*res).c_str(), alloc), alloc);

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string metaPath = Access::Path(*res) + ".meta";
    std::ofstream ofs(metaPath, std::ios::trunc);
    if (!ofs.is_open()) return false;
    ofs << buffer.GetString();
    return true;
}



bool MetaIO::LoadSimpleMeta(std::shared_ptr<Game_Resource>& res)
{
    if (!res) return false;
    std::string metaPath = Access::Path(*res) + ".meta";
    if (!std::filesystem::exists(metaPath))
        return false;

    std::ifstream ifs(metaPath);
    if (!ifs.is_open()) return false;

    std::string json((std::istreambuf_iterator<char>(ifs)),
        std::istreambuf_iterator<char>());
    Document doc;
    if (doc.Parse(json.c_str()).HasParseError())
        return false;

    if (doc.HasMember("guid"))
        Access::SetGUID(*res, doc["guid"].GetString());
    if (doc.HasMember("alias"))
        Access::SetAlias(*res, doc["alias"].GetString());

    return true;
}

bool MetaIO::SaveFbxMeta(const FbxMeta& meta)
{
    Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("guid", Value(meta.guid.c_str(), alloc), alloc);
    doc.AddMember("path", Value(meta.path.c_str(), alloc), alloc);
    doc.AddMember("type", Value("FBX_MODEL", alloc), alloc);

    Value subArr(kArrayType);
    for (auto& s : meta.sub_resources)
    {
        Value obj(kObjectType);
        obj.AddMember("name", Value(s.name.c_str(), alloc), alloc);
        obj.AddMember("type", Value(s.type.c_str(), alloc), alloc);
        obj.AddMember("guid", Value(s.guid.c_str(), alloc), alloc);
        subArr.PushBack(obj, alloc);
    }
    doc.AddMember("sub_resources", subArr, alloc);

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string metaPath = meta.path + ".meta";
    std::ofstream ofs(metaPath, std::ios::trunc);
    if (!ofs.is_open()) return false;
    ofs << buffer.GetString();
    return true;
}

bool MetaIO::LoadFbxMeta(FbxMeta& out, const std::string& fbxPath)
{
    std::ifstream ifs(fbxPath + ".meta");
    if (!ifs.is_open()) return false;

    std::string json((std::istreambuf_iterator<char>(ifs)),
        std::istreambuf_iterator<char>());
    Document doc;
    if (doc.Parse(json.c_str()).HasParseError())
        return false;

    if (!doc.HasMember("guid")) return false;

    out.guid = doc["guid"].GetString();
    out.path = doc["path"].GetString();

    if (doc.HasMember("sub_resources"))
    {
        const Value& arr = doc["sub_resources"];
        for (auto& v : arr.GetArray())
        {
            SubResourceMeta s;
            s.name = v["name"].GetString();
            s.type = v["type"].GetString();
            s.guid = v["guid"].GetString();
            out.sub_resources.push_back(s);
        }
    }
    return true;
}
