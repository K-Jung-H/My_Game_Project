
constexpr UINT MAX_CBV = 1000;
constexpr UINT MAX_SRV = 1000;
constexpr UINT MAX_UAV = 1000;

enum class HeapRegion
{
    SRV_Texture,   // 일반 텍스처 
    SRV_Frame,     // 프레임 버퍼 리소스
    CBV,
    UAV,
    RTV,
    DSV
};

struct RegionAllocator
{
    UINT start;
    UINT capacity;
    UINT next = 0;
    std::queue<UINT> freeList;
};

class DescriptorManager
{
public:
    DescriptorManager(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors);

    UINT Allocate(HeapRegion region);
    void FreeDeferred(HeapRegion region, UINT index);
    void Update();

    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetRegionStartHandle(HeapRegion region) const;

    ID3D12DescriptorHeap* GetHeap() const { return mHeap.Get(); }

private:
    ComPtr<ID3D12DescriptorHeap> mHeap;
    UINT mDescriptorSize = 0;
    UINT mNumDescriptors = 0;
    D3D12_DESCRIPTOR_HEAP_TYPE mHeapType;

    UINT mNext = 0;
    std::queue<UINT> mFreeList;

    std::unordered_map<HeapRegion, RegionAllocator> mRegions;

    struct PendingFree
    {
        HeapRegion region;
        UINT index;
    };

    std::vector<PendingFree> mPendingFreeList;
    std::mutex mMutex;
};