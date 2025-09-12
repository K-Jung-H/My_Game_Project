#include "pch.h"
#include "DescriptorManager.h"

DescriptorManager::DescriptorManager(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;
    desc.Flags = (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap));
    if (FAILED(hr))
        throw std::runtime_error("Failed to create descriptor heap");


    mDescriptorSize = device->GetDescriptorHandleIncrementSize(type);
    mNumDescriptors = numDescriptors;

    for (UINT i = 0; i < numDescriptors; i++)
        mFreeList.push(i);
}

UINT DescriptorManager::Allocate()
{
    if (mFreeList.empty())
        throw std::runtime_error("Descriptor heap exhausted!");

    UINT index = mFreeList.front();
    mFreeList.pop();
    return index;
}

void DescriptorManager::FreeDeferred(UINT index)
{
    mPendingFree.push_back(index);
}

void DescriptorManager::Update()
{
    for (auto idx : mPendingFree)
        mFreeList.push(idx);
    mPendingFree.clear();
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
