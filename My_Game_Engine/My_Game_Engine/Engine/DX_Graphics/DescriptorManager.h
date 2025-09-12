#include "pch.h"

class DescriptorManager
{
public:
    DescriptorManager(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors);

    UINT Allocate();
    void FreeDeferred(UINT index);
    void Update();

    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT index) const;

    ID3D12DescriptorHeap* GetHeap() const { return mHeap.Get(); }

private:
    ComPtr<ID3D12DescriptorHeap> mHeap;
    UINT mDescriptorSize = 0;
    UINT mNumDescriptors = 0;

    std::queue<UINT> mFreeList;
    std::vector<UINT> mPendingFree;
};