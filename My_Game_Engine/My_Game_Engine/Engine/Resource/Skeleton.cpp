#include "Skeleton.h"

Skeleton::Skeleton()
    : Game_Resource(ResourceType::Skeleton)
{
}

const std::vector<BoneInfo>& Skeleton::GetBones() const
{
    return mBones;
}

const BoneInfo& Skeleton::GetBone(int index) const
{
    return mBones[index];
}

const BoneInfo& Skeleton::GetBone(const std::string& name) const
{
    int idx = GetBoneIndex(name);
    if (idx != -1)
    {
        return mBones[idx];
    }

    static const BoneInfo emptyBone = { -1 };
    return emptyBone;
}

UINT Skeleton::GetBoneCount() const
{
    return (UINT)mBones.size();
}

const std::string& Skeleton::GetBoneName(int index) const
{
    return mNames[index];
}

int Skeleton::GetBoneIndex(const std::string& name) const
{
    auto it = mNameToIndex.find(name);
    if (it != mNameToIndex.end())
        return it->second;
    return -1;
}

int Skeleton::GetRootBoneIndex() const
{
    if (mCachedRootIndex != -1)
        return mCachedRootIndex;

    for (int i = 0; i < mBones.size(); ++i)
    {
        if (mBones[i].parentIndex == -1)
        {
            const_cast<Skeleton*>(this)->mCachedRootIndex = i;
            return i;
        }
    }
    return 0; 
}

void Skeleton::BuildNameToIndex()
{
    mNameToIndex.clear();
    for (size_t i = 0; i < mNames.size(); ++i)
    {
        mNameToIndex[mNames[i]] = static_cast<int>(i);
    }
}

bool Skeleton::LoadFromFile(std::string path, const RendererContext& ctx)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) return false;

    std::string json((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    Document doc;
    if (doc.Parse(json.c_str()).HasParseError()) return false;

    mNames.clear();
    mBones.clear();
    mNameToIndex.clear();

    if (doc.HasMember("BoneList") && doc["BoneList"].IsArray())
    {
        const auto& arr = doc["BoneList"].GetArray();
        mNames.reserve(arr.Size());
        mBones.reserve(arr.Size());

        for (auto& entry : arr)
        {
            if (entry.HasMember("name"))
            {
                mNames.push_back(entry["name"].GetString());
            }
            else
            {
                mNames.push_back("Unknown");
            }

            BoneInfo info;
            info.parentIndex = entry["parentIndex"].GetInt();

            const auto& bl = entry["bindLocal"].GetArray();
            info.bindLocal = XMFLOAT4X4(
                bl[0].GetFloat(), bl[1].GetFloat(), bl[2].GetFloat(), bl[3].GetFloat(),
                bl[4].GetFloat(), bl[5].GetFloat(), bl[6].GetFloat(), bl[7].GetFloat(),
                bl[8].GetFloat(), bl[9].GetFloat(), bl[10].GetFloat(), bl[11].GetFloat(),
                bl[12].GetFloat(), bl[13].GetFloat(), bl[14].GetFloat(), bl[15].GetFloat()
            );

            XMMATRIX matBind = XMLoadFloat4x4(&info.bindLocal);
            XMVECTOR s, r, t;
            if (XMMatrixDecompose(&s, &r, &t, matBind))
            {
                XMStoreFloat3(&info.bindScale, s);
                XMStoreFloat4(&info.bindRotation, r);
                XMStoreFloat3(&info.bindTranslation, t);
            }
            else
            {
                info.bindScale = { 1.0f, 1.0f, 1.0f };
                info.bindRotation = { 0.0f, 0.0f, 0.0f, 1.0f };
                info.bindTranslation = { 0.0f, 0.0f, 0.0f };
            }

            const auto& ib = entry["inverseBind"].GetArray();
            info.inverseBind = XMFLOAT4X4(
                ib[0].GetFloat(), ib[1].GetFloat(), ib[2].GetFloat(), ib[3].GetFloat(),
                ib[4].GetFloat(), ib[5].GetFloat(), ib[6].GetFloat(), ib[7].GetFloat(),
                ib[8].GetFloat(), ib[9].GetFloat(), ib[10].GetFloat(), ib[11].GetFloat(),
                ib[12].GetFloat(), ib[13].GetFloat(), ib[14].GetFloat(), ib[15].GetFloat()
            );

            mBones.push_back(info);
        }
    }

    SortBoneList();
    return true;
}

bool Skeleton::SaveToFile(const std::string& path) const
{
    Document doc(kObjectType);
    auto& alloc = doc.GetAllocator();

    Value nameList(kArrayType);
    for (const auto& name : mNames)
    {
        nameList.PushBack(Value(name.c_str(), alloc), alloc);
    }
    doc.AddMember("BoneNameList", nameList, alloc);


    Value boneList(kArrayType);
    for (size_t i = 0; i < mBones.size(); i++)
    {
        Value entry(kObjectType);

        entry.AddMember("name", Value(mNames[i].c_str(), alloc), alloc);

        const BoneInfo& info = mBones[i];
        entry.AddMember("parentIndex", info.parentIndex, alloc);

        Value bl(kArrayType);
        const float* pBl = reinterpret_cast<const float*>(&info.bindLocal);
        for (int k = 0; k < 16; k++) bl.PushBack(pBl[k], alloc);
        entry.AddMember("bindLocal", bl, alloc);

        Value ib(kArrayType);
        const float* pIb = reinterpret_cast<const float*>(&info.inverseBind);
        for (int k = 0; k < 16; k++) ib.PushBack(pIb[k], alloc);
        entry.AddMember("inverseBind", ib, alloc);

        boneList.PushBack(entry, alloc);
    }
    doc.AddMember("BoneList", boneList, alloc);

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
    if (mBones.empty()) return;

    struct SortHelper
    {
        int originalIdx;
        std::string name;
        BoneInfo info;
    };

    std::vector<SortHelper> helpers;
    helpers.reserve(mBones.size());

    for (size_t i = 0; i < mBones.size(); ++i)
    {
        helpers.push_back({ (int)i, mNames[i], mBones[i] });
    }

    std::vector<SortHelper> sorted;
    sorted.reserve(helpers.size());

    std::vector<int> oldToNewIndex(helpers.size(), -1);
    std::vector<bool> visited(helpers.size(), false);

    std::function<void(int)> visit = [&](int currentIdx)
        {
            if (visited[currentIdx]) return;

            int parent = helpers[currentIdx].info.parentIndex;

            if (parent >= 0 && !visited[parent])
            {
                visit(parent);
            }

            visited[currentIdx] = true;

            SortHelper newItem = helpers[currentIdx];
            if (parent >= 0)
            {
                newItem.info.parentIndex = oldToNewIndex[parent];
            }

            oldToNewIndex[currentIdx] = (int)sorted.size();
            sorted.push_back(newItem);
        };

    for (size_t i = 0; i < helpers.size(); ++i)
    {
        if (helpers[i].info.parentIndex == -1)
            visit((int)i);
    }
    for (size_t i = 0; i < helpers.size(); ++i)
    {
        if (!visited[i])
            visit((int)i);
    }

    mNames.clear();
    mBones.clear();

    for (const auto& item : sorted)
    {
        mNames.push_back(item.name);
        mBones.push_back(item.info);
    }

    BuildNameToIndex();
}