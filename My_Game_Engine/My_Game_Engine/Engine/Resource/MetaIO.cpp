#include "MetaIO.h"
#include "Game_Resource.h"

static uint64_t SimpleHash(const std::string& key)
{
    uint64_t hash = 1469598103934665603ull;
    for (unsigned char c : key)
        hash = (hash ^ c) * 1099511628211ull; 
    return hash;
}

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

std::string MetaIO::CreateGUID(const std::string& path, const std::string& name)
{
    std::string key = path + "|" + name;

    uint64_t h = SimpleHash(key);

    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << h;

    return ss.str();
}

void MetaIO::EnsureResourceGUID(const std::shared_ptr<Game_Resource>& res)
{
    if (!res) return;

    std::string metaPath = res->GetPathCopy() + ".meta";

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
                res->SetGUID(doc["guid"].GetString());
                return;
            }
        }
    }

    const std::string path = res->GetPathCopy();
    const std::string name = res->GetAlias();
    FileCategory category = DetectFileCategory(path);

    switch (category)
    {
    case FileCategory::ComplexModel:
        res->SetGUID(CreateGUID(path, name));
        break;

    case FileCategory::Texture:
    case FileCategory::Material:
        res->SetGUID(CreateGUID());
        break;

    default:
        res->SetGUID(CreateGUID());
        break;
    }
}


bool MetaIO::SaveSimpleMeta(const std::shared_ptr<Game_Resource>& res)
{
    if (!res) return false;

    Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("guid", Value(res->GetGUID().c_str(), alloc), alloc);

    std::string typeStr;
    switch (res->Get_Type())
    {
    case ResourceType::Mesh: typeStr = "Mesh"; break;
    case ResourceType::Material: typeStr = "Material"; break;
    case ResourceType::Texture: typeStr = "Texture"; break;
    default: typeStr = "etc"; break;
    }
    doc.AddMember("type", Value(typeStr.c_str(), alloc), alloc);
    doc.AddMember("path", Value(res->GetPathCopy().c_str(), alloc), alloc);
    doc.AddMember("alias", Value(res->GetAlias().data(), alloc), alloc);

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string metaPath = res->GetPathCopy() + ".meta";
    std::ofstream ofs(metaPath, std::ios::trunc);
    if (!ofs.is_open()) return false;
    ofs << buffer.GetString();
    return true;
}



bool MetaIO::LoadSimpleMeta(const std::shared_ptr<Game_Resource>& res)
{
    if (!res) return false;
    std::string metaPath = res->GetPathCopy() + ".meta";
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
        res->SetGUID(doc["guid"].GetString());
    if (doc.HasMember("alias"))
        res->SetAlias(doc["alias"].GetString());

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
