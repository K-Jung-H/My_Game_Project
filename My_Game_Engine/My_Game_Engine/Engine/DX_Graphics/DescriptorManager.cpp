#include "DescriptorManager.h"

DescriptorManager::DescriptorManager(ID3D12Device* device,
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    UINT numDescriptors)
    : mHeapType(type), mNumDescriptors(numDescriptors)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = type;
    desc.NumDescriptors = numDescriptors;
    desc.Flags = (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap))))
        throw std::runtime_error("Failed to create descriptor heap");

    mDescriptorSize = device->GetDescriptorHandleIncrementSize(type);

    if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        UINT offset = 0;

        mRegions[HeapRegion::SRV_Texture] = { offset, MAX_SRV_TEXTURE, 0 };
        offset += MAX_SRV_TEXTURE;

        mRegions[HeapRegion::SRV_Frame] = { offset, MAX_SRV_FRAME, 0 };
        offset += MAX_SRV_FRAME;

        mRegions[HeapRegion::CBV] = { offset, MAX_CBV, 0 };
        offset += MAX_CBV;

        mRegions[HeapRegion::UAV] = { offset, MAX_UAV, 0 };
        offset += MAX_UAV;

        mRegions[HeapRegion::SRV_ShadowMap_Spot] = { offset, MAX_SHADOW_SPOT, 0 };
        offset += MAX_SHADOW_SPOT;

        mRegions[HeapRegion::SRV_ShadowMap_CSM] = { offset, MAX_SHADOW_CSM, 0 };
        offset += MAX_SHADOW_CSM;

        mRegions[HeapRegion::SRV_ShadowMap_Point] = { offset, MAX_SHADOW_POINT, 0 };
        offset += MAX_SHADOW_POINT;
    }
    else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
    {
        mRegions[HeapRegion::RTV] = { 0, numDescriptors, 0 };
    }
    else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
    {
        mRegions[HeapRegion::DSV] = { 0, numDescriptors, 0 };
    }
}

UINT DescriptorManager::Allocate(HeapRegion region)
{
    auto& r = mRegions[region];
    if (!r.freeList.empty())
    {
        UINT idx = r.freeList.front();
        r.freeList.pop();
        return idx;
    }
    if (r.next < r.capacity)
    {
        return r.start + r.next++;
    }
    throw std::runtime_error("Descriptor heap region exhausted!");
}

void DescriptorManager::FreeDeferred(HeapRegion region, UINT index)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mPendingFreeList.push_back({ region, index });
}

void DescriptorManager::Update()
{
    std::lock_guard<std::mutex> lock(mMutex);
    for (auto& entry : mPendingFreeList)
    {
        mRegions[entry.region].freeList.push(entry.index);
    }
    mPendingFreeList.clear();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorManager::GetCpuHandle(UINT index) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = mHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += index * mDescriptorSize;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::GetGpuHandle(UINT index) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = mHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += index * mDescriptorSize;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::GetRegionStartHandle(HeapRegion region) const
{
    auto it = mRegions.find(region);
    if (it == mRegions.end())
        throw std::runtime_error("Region not initialized");
    return GetGpuHandle(it->second.start);
}