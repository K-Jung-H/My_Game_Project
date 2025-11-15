#include "Model_Avatar.h"

Model_Avatar::Model_Avatar() : Game_Resource(ResourceType::ModelAvatar)
{
}

void Model_Avatar::SetDefinitionType(DefinitionType type)
{
    mDefinitionType = type;
}

void Model_Avatar::AutoMap(std::shared_ptr<Skeleton> skeleton)
{
    // (구현 보류)
    // mBoneMap.clear();
    // if (mDefinitionType == DefinitionType::None || !skeleton) return;
    // ...
    // (나중에 여기에 자동 추측 로직 구현)
    // ...
    OutputDebugStringA(("[Model_Avatar] AutoMap: " + GetAlias() + " - (구현 보류됨)\n").c_str());
}

const std::string& Model_Avatar::GetMappedBoneName(const std::string& abstractKey) const
{
    static const std::string emptyString = "";
    auto it = mBoneMap.find(abstractKey);
    if (it != mBoneMap.end())
    {
        return it->second;
    }
    return emptyString;
}

bool Model_Avatar::SaveToFile(const std::string& outputPath) const
{
    using namespace rapidjson;
    Document doc(kObjectType);
    Document::AllocatorType& alloc = doc.GetAllocator();

    doc.AddMember("DefinitionType", (int)mDefinitionType, alloc);

    Value mapping(kArrayType);
    for (const auto& [key, value] : mBoneMap)
    {
        Value entry(kObjectType);
        entry.AddMember("key", Value(key.c_str(), alloc), alloc);
        entry.AddMember("value", Value(value.c_str(), alloc), alloc);
        mapping.PushBack(entry, alloc);
    }
    doc.AddMember("Mapping", mapping, alloc);

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream ofs(GetPath());
    if (!ofs.is_open())
    {
        OutputDebugStringA(("[Model_Avatar] Error: " + GetPath() + " 파일 쓰기 실패.\n").c_str());
        return false;
    }

    ofs << buffer.GetString();
    ofs.close();
    return true;
}

bool Model_Avatar::LoadFromFile(std::string path, const RendererContext& ctx)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) return false;

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    std::string json = buffer.str();
    ifs.close();

    rapidjson::Document doc;
    if (doc.Parse(json.c_str()).HasParseError())
    {
        return false;
    }

    if (doc.HasMember("DefinitionType"))
    {
        mDefinitionType = (DefinitionType)doc["DefinitionType"].GetInt();
    }

    mBoneMap.clear();
    if (doc.HasMember("Mapping") && doc["Mapping"].IsArray())
    {
        for (auto& entry : doc["Mapping"].GetArray())
        {
            if (entry.IsObject() && entry.HasMember("key") && entry.HasMember("value"))
            {
                mBoneMap[entry["key"].GetString()] = entry["value"].GetString();
            }
        }
    }
    return true;
}