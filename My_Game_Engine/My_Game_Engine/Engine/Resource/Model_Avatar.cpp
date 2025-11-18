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
    {
        OutputDebugStringA("[AutoMap] Invalid skeleton or definition.\n");
        return;
    }

    auto definitionMgr = GameEngine::Get().GetAvatarSystem();
    auto definition = definitionMgr->GetDefinition(mDefinitionType);

    if (!definition)
    {
        OutputDebugStringA("[AutoMap] Definition not found.\n");
        return;
    }

    const auto& defs = definition->GetBoneDefinitions();
    const auto& bones = skeleton->GetBoneList();

    std::vector<std::string> lowerNames;
    lowerNames.reserve(bones.size());
    for (auto& b : bones)
        lowerNames.push_back(ToLower(b.name));

    std::unordered_set<int> used;

    auto isBlacklisted = [](const std::string& s) -> bool
        {
            static const char* BAD[] = {
                "hair", "tail", "cloth", "skirt", "dress", "hood", "cape",
                "coat", "jacket", "fur", "ear", "wing", "helmet", "mask", "braid"
            };
            for (auto& k : BAD)
                if (s.find(k) != std::string::npos)
                    return true;
            return false;
        };

    auto toLowerVec = [&](const std::vector<std::string>& src)
        {
            std::vector<std::string> out;
            out.reserve(src.size());
            for (auto& s : src)
                out.push_back(ToLower(s));
            return out;
        };

    auto containsAny = [&](const std::string& name, const std::vector<std::string>& words)
        {
            for (auto& w : words)
                if (name.find(w) != std::string::npos)
                    return true;
            return false;
        };

    const std::vector<std::string> ARM = { "arm", "upperarm", "lowerarm", "forearm", "shoulder" };
    const std::vector<std::string> LEG = { "leg", "upperleg", "lowerleg", "upleg", "calf", "thigh" };
    const std::vector<std::string> FOOT = { "foot" };
    const std::vector<std::string> TOE = { "toe" };
    const std::vector<std::string> HAND = { "hand" };
    const std::vector<std::string> HEAD = { "head" };
    const std::vector<std::string> NECK = { "neck" };
    const std::vector<std::string> SPINE = { "spine", "chest" };

    auto findBone = [&](const BoneDefinition& def) -> int
        {
            std::string key = ToLower(def.key);
            auto kw = toLowerVec(def.keywords);

            bool needLeft = key.find("left") != std::string::npos;
            bool needRight = key.find("right") != std::string::npos;

            bool tagArm = containsAny(key, ARM);
            bool tagLeg = containsAny(key, LEG);
            bool tagFoot = containsAny(key, FOOT);
            bool tagToe = containsAny(key, TOE);
            bool tagHand = containsAny(key, HAND);
            bool tagHead = containsAny(key, HEAD);
            bool tagNeck = containsAny(key, NECK);
            bool tagSpine = containsAny(key, SPINE);

            bool tagUpper = (key.find("upper") != std::string::npos || key.find("upleg") != std::string::npos);
            bool tagLower = (key.find("lower") != std::string::npos);

            int bestScore = -9999;
            int bestIdx = -1;

            for (int i = 0; i < (int)bones.size(); i++)
            {
                if (used.count(i)) continue;

                const std::string& name = lowerNames[i];

                if (isBlacklisted(name))
                    continue;

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
                bool semanticHit = false;

                for (auto& w : kw)
                {
                    if (name == w)           score += 50, semanticHit = true;
                    else if (name.find(w) != std::string::npos)
                        score += 12, semanticHit = true;
                }

                auto tagMatch = [&](bool tag, const std::vector<std::string>& list)
                    {
                        if (!tag) return;
                        if (containsAny(name, list))
                        {
                            score += 10;
                            semanticHit = true;
                        }
                    };
                tagMatch(tagArm, ARM);
                tagMatch(tagLeg, LEG);
                tagMatch(tagFoot, FOOT);
                tagMatch(tagToe, TOE);
                tagMatch(tagHand, HAND);
                tagMatch(tagHead, HEAD);
                tagMatch(tagNeck, NECK);
                tagMatch(tagSpine, SPINE);

                if (tagUpper && (name.find("up") != std::string::npos || name.find("upper") != std::string::npos))
                    score += 4;
                if (tagLower && name.find("lower") != std::string::npos)
                    score += 4;

                if (!semanticHit)
                    continue;

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

    for (auto& def : defs)
    {
        int idx = findBone(def);
        if (idx != -1)
        {
            mBoneMap[def.key] = bones[idx].name;
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