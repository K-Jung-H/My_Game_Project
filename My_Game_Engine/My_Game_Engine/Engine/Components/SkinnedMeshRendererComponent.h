#pragma once
#include "MeshRendererComponent.h"
#include "AnimationControllerComponent.h"

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
    virtual rapidjson::Value ToJSON(rapidjson::Document::AllocatorType& alloc) const;
    virtual void FromJSON(const rapidjson::Value& val);

public:
    static constexpr Component_Type Type = Component_Type::Skinned_Mesh_Renderer;
    Component_Type GetType() const override { return Type; }

public:
    SkinnedMeshRendererComponent();
    virtual ~SkinnedMeshRendererComponent() = default;



    void Initialize();
    virtual void SetMesh(UINT id) override;
    virtual void Update() override;

    FrameSkinBuffer& GetFrameSkinBuffer(UINT frameIndex) { return mFrameSkinnedBuffers[frameIndex]; }

    const D3D12_VERTEX_BUFFER_VIEW& GetSkinnedVBV(UINT frameIndex) const;

    std::shared_ptr<AnimationControllerComponent> GetAnimController() const { return mCachedAnimController; }
    bool HasValidBuffers() const;

private:
    void CacheAnimController();
    void CreatePreSkinnedOutputBuffers(std::shared_ptr<SkinnedMesh> skinnedMesh);

    void ApplyDeferredMeshChange();

private:
    std::shared_ptr<AnimationControllerComponent> mCachedAnimController;

    FrameSkinBuffer mFrameSkinnedBuffers[Engine::Frame_Render_Buffer_Count];
    bool mHasSkinnedBuffer = false;

    D3D12_VERTEX_BUFFER_VIEW mOriginalHotVBV;

    bool mDeferredMeshUpdate = false;
    UINT mDeferredMeshId = Engine::INVALID_ID;
};