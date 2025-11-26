#include "AvatarSystem.h"

bool Engine_AvatarDefinition::LoadFromFile(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open())
    {
        OutputDebugStringA(("[Engine_AvatarDefinition] Error: " + path + " 파일을 열 수 없습니다.\n").c_str());
        return false;
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    std::string json = buffer.str();
    ifs.close();

    rapidjson::Document doc;
    if (doc.Parse(json.c_str()).HasParseError())
    {
        OutputDebugStringA(("[Engine_AvatarDefinition] Error: " + path + " JSON 파싱 실패.\n").c_str());
        return false;
    }

    if (doc.HasMember("DefinitionType") && doc["DefinitionType"].IsString())
    {
        mDefinitionType = StringToDefinitionType(doc["DefinitionType"].GetString());
    }

    if (doc.HasMember("BoneDefinitions") && doc["BoneDefinitions"].IsArray())
    {
        for (const auto& def : doc["BoneDefinitions"].GetArray())
        {
            if (def.IsObject() && def.HasMember("key") && def.HasMember("keywords"))
            {
                BoneDefinition boneDef;
                boneDef.key = def["key"].GetString();

                for (const auto& kw : def["keywords"].GetArray())
                {
                    if (kw.IsString())
                    {
                        std::string s = kw.GetString();
                        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
                        boneDef.keywords.push_back(s);
                    }
                }
                mBoneDefinitions.push_back(std::move(boneDef));
            }
        }
    }

    if (mDefinitionType == DefinitionType::None || mBoneDefinitions.empty())
    {
        OutputDebugStringA(("[Engine_AvatarDefinition] Error: " + path + " 파일 내용이 유효하지 않습니다.\n").c_str());
        return false;
    }

    return true;
}

Engine_Humanoid_AvatarDefinition::Engine_Humanoid_AvatarDefinition() : Engine_AvatarDefinition(DefinitionType::Humanoid)
{

}

Engine_Animal_AvatarDefinition::Engine_Animal_AvatarDefinition() : Engine_AvatarDefinition(DefinitionType::Animal)
{

}

//========================================================

void AvatarDefinitionManager::Initialize(const std::string& definitionFolderPath)
{
    namespace fs = std::filesystem;
    mDefinitions.clear();

    for (const auto& entry : fs::directory_iterator(definitionFolderPath))
    {
        if (entry.path().extension() == ".json")
        {
            auto def = std::make_shared<Engine_AvatarDefinition>(DefinitionType::None);

            if (def->LoadFromFile(entry.path().string()))
            {
                mDefinitions[def->GetType()] = def;
            }
        }
    }
}

Engine_AvatarDefinition* AvatarDefinitionManager::GetDefinition(DefinitionType type)
{
    auto it = mDefinitions.find(type);
    if (it != mDefinitions.end())
        return it->second.get();
    return nullptr;
}