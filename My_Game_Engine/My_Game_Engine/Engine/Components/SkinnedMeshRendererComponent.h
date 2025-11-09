#pragma once
#include "MeshRendererComponent.h"


class AnimationController;

struct FrameSkinBuffer
{
    ComPtr<ID3D12Resource> skinnedBuffer;
    ComPtr<ID3D12Resource> uploadBuffer;
    D3D12_VERTEX_BUFFER_VIEW vbv;
    UINT uavSlot = UINT_MAX;
    bool mIsSkinningResultReady = false;
};

class SkinnedMeshRendererComponent : public MeshRendererComponent
{
public:
    SkinnedMeshRendererComponent() = default;
    virtual ~SkinnedMeshRendererComponent() = default;

    void Initialize();
    virtual void SetMesh(UINT id);


    FrameSkinBuffer& GetFrameSkinBuffer(UINT frameIndex) { return mFrameSkinnedBuffers[frameIndex]; }

    const D3D12_VERTEX_BUFFER_VIEW& GetSkinnedVBV(UINT frameIndex) const;

    AnimationController* GetAnimController() const { return mCachedAnimController; }

private:
    void CacheAnimController();

    void CreatePreSkinnedOutputBuffers(std::shared_ptr<SkinnedMesh> skinnedMesh);

private:
    AnimationController* mCachedAnimController = nullptr;

    FrameSkinBuffer mFrameSkinnedBuffers[Engine::Frame_Render_Buffer_Count];
    bool mHasSkinnedBuffer = false;

    D3D12_VERTEX_BUFFER_VIEW mOriginalHotVBV;
};