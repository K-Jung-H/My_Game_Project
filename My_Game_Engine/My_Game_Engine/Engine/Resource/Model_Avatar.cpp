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
    if (!skeleton || mDefinitionType == DefinitionType::None)
        return;

    auto defMgr = GameEngine::Get().GetAvatarSystem();
    auto definition = defMgr->GetDefinition(mDefinitionType);
    if (!definition)
        return;

    const auto defs = definition->GetBoneDefinitions();

    UINT boneCount = skeleton->GetBoneCount();

    std::vector<std::string> lowerNames;
    lowerNames.reserve(boneCount);
    for (UINT i = 0; i < boneCount; ++i)
    {
        lowerNames.push_back(ToLower(skeleton->GetBoneName(i)));
    }

    std::unordered_set<int> used;

    auto toLowerVec = [&](const std::vector<std::string>& src)
        {
            std::vector<std::string> out;
            out.reserve(src.size());
            for (auto& s : src) out.push_back(ToLower(s));
            return out;
        };

    auto containsAny = [&](const std::string& name, const std::vector<std::string>& list)
        {
            for (auto& s : list)
                if (name.find(s) != std::string::npos)
                    return true;
            return false;
        };

    const std::vector<std::string> FOREARM_SET = {
        "forearm", "lowerarm", "elbow", "ulna", "_twist", "twist"
    };

    auto isForeArmName = [&](const std::string& n)
        {
            return containsAny(n, FOREARM_SET);
        };

    std::vector<BoneDefinition> orderedDefs = defs;
    std::stable_sort(orderedDefs.begin(), orderedDefs.end(),
        [&](const BoneDefinition& a, const BoneDefinition& b)
        {
            const std::string A = ToLower(a.key);
            const std::string B = ToLower(b.key);
            bool aLow = A.find("lowerarm") != std::string::npos ||
                A.find("forearm") != std::string::npos ||
                A.find("elbow") != std::string::npos;
            bool bLow = B.find("lowerarm") != std::string::npos ||
                B.find("forearm") != std::string::npos ||
                B.find("elbow") != std::string::npos;
            if (aLow != bLow) return aLow;
            return false;
        }
    );

    auto findBone = [&](const BoneDefinition& def)
        {
            std::string key = ToLower(def.key);
            auto kw = toLowerVec(def.keywords);
            bool needLeft = key.find("left") != std::string::npos;
            bool needRight = key.find("right") != std::string::npos;

            bool isLowerArm = key.find("lowerarm") != std::string::npos ||
                key.find("forearm") != std::string::npos ||
                key.find("elbow") != std::string::npos;

            bool isUpperArm = key.find("upperarm") != std::string::npos &&
                (key.find("lowerarm") == std::string::npos &&
                    key.find("forearm") == std::string::npos);

            int bestIdx = -1;
            int bestScore = -9999;

            for (int i = 0; i < (int)boneCount; i++)
            {
                if (used.count(i)) continue;

                const std::string& name = lowerNames[i];

                if (needLeft)
                {
                    if (!(name.find("left") != std::string::npos ||
                        name.find("_l") != std::string::npos ||
                        name.find(".l") != std::string::npos))
                        continue;
                }
                if (needRight)
                {
                    if (!(name.find("right") != std::string::npos ||
                        name.find("_r") != std::string::npos ||
                        name.find(".r") != std::string::npos))
                        continue;
                }

                int score = 0;

                for (auto& w : kw)
                {
                    if (name == w) score += 50;
                    else if (name.find(w) != std::string::npos) score += 12;
                }

                bool boneIsFore = isForeArmName(name);

                if (isLowerArm)
                {
                    if (boneIsFore) score += 100;
                    else score -= 999;
                }

                if (isUpperArm)
                {
                    if (boneIsFore) score -= 500;
                }

                if (score > bestScore)
                {
                    bestScore = score;
                    bestIdx = i;
                }
            }

            if (bestScore < 5)
                return -1;

            return bestIdx;
        };

    for (auto& def : orderedDefs)
    {
        int idx = findBone(def);
        if (idx != -1)
        {
            mBoneMap[def.key] = skeleton->GetBoneName(idx);
            used.insert(idx);
        }
    }

    OutputDebugStringA(("[AutoMap] Final mapped bones: " + std::to_string(mBoneMap.size()) + "\n").c_str());
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