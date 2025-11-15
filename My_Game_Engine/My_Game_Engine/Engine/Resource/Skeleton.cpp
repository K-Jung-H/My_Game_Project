#include "Skeleton.h"

bool Skeleton::LoadFromFile(std::string path, const RendererContext& ctx)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) return false;
    std::string json((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    Document doc;
    if (doc.Parse(json.c_str()).HasParseError()) return false;

    BoneList.clear();
    if (doc.HasMember("BoneList") && doc["BoneList"].IsArray())
    {
        for (auto& entry : doc["BoneList"].GetArray())
        {
            Bone bone;
            bone.name = entry["name"].GetString();
            bone.parentIndex = entry["parentIndex"].GetInt();

            const auto& m = entry["inverseBind"].GetArray();
            bone.inverseBind = XMFLOAT4X4(
                m[0].GetFloat(), m[1].GetFloat(), m[2].GetFloat(), m[3].GetFloat(),
                m[4].GetFloat(), m[5].GetFloat(), m[6].GetFloat(), m[7].GetFloat(),
                m[8].GetFloat(), m[9].GetFloat(), m[10].GetFloat(), m[11].GetFloat(),
                m[12].GetFloat(), m[13].GetFloat(), m[14].GetFloat(), m[15].GetFloat()
            );
            BoneList.push_back(bone);
        }
    }
    BuildNameToIndex();
    return true;
}

bool Skeleton::SaveToFile(const std::string& path) const
{
    Document doc(kObjectType);
    auto& alloc = doc.GetAllocator();

    Value bones(kArrayType);
    for (const auto& bone : BoneList)
    {
        Value entry(kObjectType);
        entry.AddMember("name", Value(bone.name.c_str(), alloc), alloc);
        entry.AddMember("parentIndex", bone.parentIndex, alloc);

        Value m(kArrayType);
        for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c) m.PushBack(bone.inverseBind.m[r][c], alloc);
        entry.AddMember("inverseBind", m, alloc);

        bones.PushBack(entry, alloc);
    }
    doc.AddMember("BoneList", bones, alloc);

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream ofs(path, std::ios::trunc);
    if (!ofs.is_open()) return false;
    ofs << buffer.GetString();
    ofs.close();
    return true;
}

void Skeleton::SortBoneList()
{
    size_t boneCount = BoneList.size();
    if (boneCount == 0) return;

    std::vector<Bone> sortedList;
    sortedList.reserve(boneCount);

    std::vector<int> oldToNewIndex(boneCount, -1);
    std::vector<bool> visited(boneCount, false);

    std::function<void(int)> visit;
    visit = [&](int oldBoneIndex)
        {
            if (oldBoneIndex == -1 || visited[oldBoneIndex])
            {
                return;
            }

            int oldParentIndex = BoneList[oldBoneIndex].parentIndex;
            if (oldParentIndex != -1)
            {
                visit(oldParentIndex);
            }

            visited[oldBoneIndex] = true;
            oldToNewIndex[oldBoneIndex] = (int)sortedList.size();
            sortedList.push_back(BoneList[oldBoneIndex]);
        };

    for (int i = 0; i < boneCount; ++i)
    {
        if (BoneList[i].parentIndex == -1)
        {
            visit(i);
        }
    }

    for (int i = 0; i < boneCount; ++i)
    {
        if (!visited[i])
        {
            visit(i);
        }
    }

    for (size_t i = 0; i < sortedList.size(); ++i)
    {
        int oldParentIndex = sortedList[i].parentIndex;
        if (oldParentIndex != -1)
        {
            sortedList[i].parentIndex = oldToNewIndex[oldParentIndex];
        }
    }

    BoneList = std::move(sortedList);
}