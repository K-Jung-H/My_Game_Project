#include "AvatarMask.h"
#include "GameEngine.h"


AvatarMask::AvatarMask()
    : Game_Resource(ResourceType::AvatarMask)
{
}

bool AvatarMask::LoadFromFile(std::string path, const RendererContext& ctx)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) return false;

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    std::string json = buffer.str();
    ifs.close();

    rapidjson::Document doc;
    if (doc.Parse(json.c_str()).HasParseError()) return false;

    mWeights.clear();

    if (doc.HasMember("Bones") && doc["Bones"].IsArray())
    {
        const auto& arr = doc["Bones"].GetArray();
        for (const auto& entry : arr)
        {
            if (entry.HasMember("Key") && entry.HasMember("Weight"))
            {
                std::string key = entry["Key"].GetString();
                float weight = entry["Weight"].GetFloat();
                mWeights[key] = weight;
            }
        }
    }

    return true;
}

bool AvatarMask::SaveToFile(const std::string& path) const
{
    std::string dirPath = "Mask";
    if (!std::filesystem::exists(dirPath))
        std::filesystem::create_directory(dirPath);

    std::string fileName = GetAlias();
    if (fileName.empty())
        fileName = "UnnamedMask";

    std::string fullPath = dirPath + "/" + fileName + ".mask";

    Document doc(kObjectType);
    Document::AllocatorType& alloc = doc.GetAllocator();

    Value bonesArr(kArrayType);
    for (const auto& [key, weight] : mWeights)
    {
        Value entry(kObjectType);
        entry.AddMember("Key", Value(key.c_str(), alloc), alloc);
        entry.AddMember("Weight", weight, alloc);
        bonesArr.PushBack(entry, alloc);
    }

    doc.AddMember("Bones", bonesArr, alloc);

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream ofs(path);
    if (!ofs.is_open()) return false;

    ofs << buffer.GetString();
    ofs.close();

    return true;
}

void AvatarMask::ExpandHierarchy(const Skeleton* skeleton, const std::vector<std::string>& boneToKeyMap)
{
    if (!skeleton) return;

    const auto& bones = skeleton->GetBones();
    size_t boneCount = bones.size();

    if (boneToKeyMap.size() != boneCount) return;

    for (size_t i = 0; i < boneCount; ++i)
    {
        const BoneInfo& bone = bones[i];

        const std::string& currentKey = boneToKeyMap[i];
        if (currentKey.empty()) continue;

        bool hasWeight = (mWeights.find(currentKey) != mWeights.end());

        if (!hasWeight && bone.parentIndex >= 0)
        {
            const std::string& parentKey = boneToKeyMap[bone.parentIndex];

            if (!parentKey.empty())
            {
                auto parentIt = mWeights.find(parentKey);
                if (parentIt != mWeights.end())
                {
                    mWeights[currentKey] = parentIt->second;
                }
            }
        }
    }
}

float AvatarMask::GetWeight(const std::string& abstractKey) const
{
    auto it = mWeights.find(abstractKey);
    if (it != mWeights.end())
    {
        return it->second;
    }
    return 0.0f;
}

void AvatarMask::SetWeight(const std::string& abstractKey, float weight)
{
    mWeights[abstractKey] = weight;
}

void AvatarMask::Clear()
{
    mWeights.clear();
}