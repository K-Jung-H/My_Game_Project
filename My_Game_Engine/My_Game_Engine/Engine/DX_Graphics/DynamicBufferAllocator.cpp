#include "DynamicBufferAllocator.h"

UploadBuffer::UploadBuffer(ID3D12Device* device, size_t size)
    : mSize(size)
{
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = size;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mBuffer));

    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create UploadBuffer");
    }

    D3D12_RANGE readRange = { 0, 0 };
    hr = mBuffer->Map(0, &readRange, &mCpuPtr);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to map UploadBuffer");
    }

    mGpuAddr = mBuffer->GetGPUVirtualAddress();
}

UploadBuffer::~UploadBuffer()
{
    if (mBuffer)
    {
        mBuffer->Unmap(0, nullptr);
    }
}

DynamicBufferAllocator::DynamicBufferAllocator(ID3D12Device* device, size_t pageSize)
    : mDevice(device), mPageSize(pageSize), mCurrentPage(nullptr), mCurrentOffset(0), mUsedPageIndex(0)
{
}

DynamicBufferAllocator::~DynamicBufferAllocator()
{
    mPages.clear();
}

Allocation DynamicBufferAllocator::Allocate(size_t size, size_t alignment)
{
    size_t alignedOffset = (mCurrentOffset + (alignment - 1)) & ~(alignment - 1);

    if (mCurrentPage == nullptr || (alignedOffset + size) > mPageSize)
    {
        mCurrentPage = CreateNewPage();
        mCurrentOffset = 0;
        alignedOffset = 0;
    }

    Allocation alloc;
    alloc.CpuAddress = static_cast<uint8_t*>(mCurrentPage->GetCpuAddress()) + alignedOffset;
    alloc.GpuAddress = mCurrentPage->GetGpuAddress() + alignedOffset;
    alloc.Offset = alignedOffset;
    alloc.Buffer = mCurrentPage->GetResource();

    mCurrentOffset = alignedOffset + size;

    return alloc;
}

void DynamicBufferAllocator::Reset()
{
    mUsedPageIndex = 0;
    mCurrentPage = nullptr;
    mCurrentOffset = 0;
}

UploadBuffer* DynamicBufferAllocator::CreateNewPage()
{
    if (mUsedPageIndex < mPages.size())
    {
        UploadBuffer* page = mPages[mUsedPageIndex].get();
        mUsedPageIndex++;
        return page;
    }

    auto newPage = std::make_unique<UploadBuffer>(mDevice, mPageSize);
    UploadBuffer* rawPtr = newPage.get();
    mPages.push_back(std::move(newPage));
    mUsedPageIndex++;

    return rawPtr;
}