#pragma once

struct Allocation
{
    void* CpuAddress = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS GpuAddress = 0;
    size_t Offset = 0;
    ID3D12Resource* Buffer = nullptr;
};

class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device* device, size_t size);
    ~UploadBuffer();

    void* GetCpuAddress() const { return mCpuPtr; }
    D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const { return mGpuAddr; }
    ID3D12Resource* GetResource() const { return mBuffer.Get(); }
    size_t GetSize() const { return mSize; }

private:
    ComPtr<ID3D12Resource> mBuffer;
    void* mCpuPtr = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS mGpuAddr = 0;
    size_t mSize = 0;
};

class DynamicBufferAllocator
{
public:
    DynamicBufferAllocator(ID3D12Device* device, size_t pageSize = 2 * 1024 * 1024);
    ~DynamicBufferAllocator();

    Allocation Allocate(size_t size, size_t alignment = 256);
    void Reset();

private:
    UploadBuffer* CreateNewPage();

private:
    ID3D12Device* mDevice = nullptr;
    size_t mPageSize = 0;

    std::vector<std::unique_ptr<UploadBuffer>> mPages;
    UploadBuffer* mCurrentPage = nullptr;
    size_t mCurrentOffset = 0;
    size_t mUsedPageIndex = 0;
};