#include "Model_Avatar.h"
#include "GameEngine.h"

Model_Avatar::Model_Avatar() : Game_Resource(ResourceType::ModelAvatar)
{
}

void Model_Avatar::SetDefinitionType(DefinitionType type)
{
    mDefinitionType = type;
}

void Model_Avatar::AutoMap(std::shared_ptr<Skeleton> skeleton)
{
    mBoneMap.clear();
    if (mDefinitionType == DefinitionType::None || !skeleton)
    {
        OutputDebugStringA("[Model_Avatar] AutoMap: DefinitionType이 None이거나 Skeleton이 Null입니다.\n");
        return;
    }

    auto definitionMgr = GameEngine::Get().GetAvatarSystem(); 
    auto definition = definitionMgr->GetDefinition(mDefinitionType);

    if (!definition)
    {
        OutputDebugStringA("[Model_Avatar] AutoMap failed: DefinitionType을 찾을 수 없음\n");
        return;
    }

    const auto& definitionList = definition->GetBoneDefinitions();
    const auto& skeletonBones = skeleton->GetBoneList();

    for (const BoneDefinition& def : definitionList)
    {
        bool foundMatch = false;
        for (const Bone& bone : skeletonBones)
        {
            std::string lowerBoneName = ToLower(bone.name);

            for (const std::string& keyword : def.keywords)
            {
                if (lowerBoneName.find(keyword) != std::string::npos)
                {
                    mBoneMap[def.key] = bone.name; 
                    foundMatch = true;
                    break;
                }
            }
            if (foundMatch) break;
        }
    }
    
    OutputDebugStringA(("[Model_Avatar] AutoMap: " + GetAlias() + " 완료. " + std::to_string(mBoneMap.size()) + "개의 뼈가 매핑됨.\n").c_str());
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