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
    mInverseBind.clear();


    if (doc.HasMember("BoneList") && doc["BoneList"].IsArray())
    {
        for (auto& entry : doc["BoneList"].GetArray())
        {
            Bone b;
            b.name = entry["name"].GetString();
            b.parentIndex = entry["parentIndex"].GetInt();


            const auto& bl = entry["bindLocal"].GetArray();
            b.bindLocal = XMFLOAT4X4(
                bl[0].GetFloat(), bl[1].GetFloat(), bl[2].GetFloat(), bl[3].GetFloat(),
                bl[4].GetFloat(), bl[5].GetFloat(), bl[6].GetFloat(), bl[7].GetFloat(),
                bl[8].GetFloat(), bl[9].GetFloat(), bl[10].GetFloat(), bl[11].GetFloat(),
                bl[12].GetFloat(), bl[13].GetFloat(), bl[14].GetFloat(), bl[15].GetFloat()
            );


            const auto& ib = entry["inverseBind"].GetArray();
            mInverseBind.emplace_back(
                ib[0].GetFloat(), ib[1].GetFloat(), ib[2].GetFloat(), ib[3].GetFloat(),
                ib[4].GetFloat(), ib[5].GetFloat(), ib[6].GetFloat(), ib[7].GetFloat(),
                ib[8].GetFloat(), ib[9].GetFloat(), ib[10].GetFloat(), ib[11].GetFloat(),
                ib[12].GetFloat(), ib[13].GetFloat(), ib[14].GetFloat(), ib[15].GetFloat()
            );


            BoneList.push_back(b);
        }
    }


    BuildNameToIndex();
    BuildBindPoseTransforms();
    return true;
}

bool Skeleton::SaveToFile(const std::string& path) const
{
    Document doc(kObjectType);
    auto& alloc = doc.GetAllocator();


    Value bones(kArrayType);
    for (size_t i = 0; i < BoneList.size(); i++)
    {
        const Bone& b = BoneList[i];


        Value entry(kObjectType);
        entry.AddMember("name", Value(b.name.c_str(), alloc), alloc);
        entry.AddMember("parentIndex", b.parentIndex, alloc);


        Value bl(kArrayType);
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                bl.PushBack(b.bindLocal.m[r][c], alloc);
        entry.AddMember("bindLocal", bl, alloc);


        Value ib(kArrayType);
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                ib.PushBack(mInverseBind[i].m[r][c], alloc);
        entry.AddMember("inverseBind", ib, alloc);


        bones.PushBack(entry, alloc);
    }
    doc.AddMember("BoneList", bones, alloc);


    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);


    std::ofstream ofs(path, std::ios::trunc);
    if (!ofs.is_open()) return false;
    ofs << buffer.GetString();
    ofs.close();
    return true;
}

void Skeleton::SortBoneList()
{
    const size_t boneCount = BoneList.size();
    if (boneCount == 0)
        return;

    std::vector<Bone> sortedList;
    sortedList.reserve(boneCount);

    std::vector<int>  oldToNewIndex(boneCount, -1);
    std::vector<bool> visited(boneCount, false);

    std::function<void(int)> visit = [&](int oldIdx)
        {
            if (oldIdx < 0 || visited[oldIdx])
                return;

            int oldParent = BoneList[oldIdx].parentIndex;
            if (oldParent >= 0)
                visit(oldParent);

            visited[oldIdx] = true;
            oldToNewIndex[oldIdx] = static_cast<int>(sortedList.size());
            sortedList.push_back(BoneList[oldIdx]);
        };

    for (int i = 0; i < static_cast<int>(boneCount); ++i)
    {
        if (BoneList[i].parentIndex == -1)
            visit(i);
    }

    for (int i = 0; i < static_cast<int>(boneCount); ++i)
    {
        if (!visited[i])
            visit(i);
    }

    for (auto& b : sortedList)
    {
        if (b.parentIndex >= 0)
            b.parentIndex = oldToNewIndex[b.parentIndex];
    }

    if (mInverseBind.size() == boneCount)
    {
        std::vector<XMFLOAT4X4> newInv(boneCount);
        for (size_t oldIdx = 0; oldIdx < boneCount; ++oldIdx)
        {
            int newIdx = oldToNewIndex[oldIdx];
            if (newIdx >= 0)
                newInv[newIdx] = mInverseBind[oldIdx];
        }
        mInverseBind.swap(newInv);
    }

    BoneList.swap(sortedList);
}


void Skeleton::BuildNameToIndex()
{
    NameToIndex.clear();
    for (size_t i = 0; i < BoneList.size(); i++)
        NameToIndex[BoneList[i].name] = static_cast<int>(i);
}

void Skeleton::BuildBindPoseTransforms()
{
    using namespace DirectX;

    const size_t boneCount = BoneList.size();
    if (boneCount == 0)
    {
        mBindLocal.clear();
        mBindGlobal.clear();
        return;
    }

    mBindLocal.resize(boneCount);
    mBindGlobal.resize(boneCount);

    for (size_t i = 0; i < boneCount; ++i)
    {
        mBindLocal[i] = BoneList[i].bindLocal;
    }

    for (size_t i = 0; i < boneCount; ++i)
    {
        const int parentIdx = BoneList[i].parentIndex;

        XMMATRIX localM = XMLoadFloat4x4(&mBindLocal[i]);
        if (parentIdx < 0)
        {
            XMStoreFloat4x4(&mBindGlobal[i], localM);
        }
        else
        {
            XMMATRIX parentGlobal = XMLoadFloat4x4(&mBindGlobal[parentIdx]);
            XMMATRIX globalM = XMMatrixMultiply(localM, parentGlobal);
            XMStoreFloat4x4(&mBindGlobal[i], globalM);
        }
    }
}