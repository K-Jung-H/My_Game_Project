#include "Skeleton.h"

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