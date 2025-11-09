#pragma once
#include "Core/Component.h"
#include "Resource/Skeleton.h"

struct BoneMatrixData
{
    XMFLOAT4X4 transform;
};

class AnimationControllerComponent : public Component
{
public:
    static constexpr Component_Type Type = Component_Type::AnimationController;
    Component_Type GetType() const override { return Type; }

public:
    AnimationControllerComponent();
    virtual ~AnimationControllerComponent() = default;

    void SetSkeleton(std::shared_ptr<Skeleton> skeleton);

    void Update(float deltaTime);
    UINT GetBoneMatrixSRV() const;

    bool IsReady() const { return mBoneMatrixSRVSlot != UINT_MAX && mSkeleton != nullptr; }
    std::shared_ptr<Skeleton> GetSkeleton() { return mSkeleton; }

private:
    void CreateBoneMatrixBuffer();
    void EvaluateAnimation(float time);

private:
    std::shared_ptr<Skeleton> mSkeleton;
    std::vector<BoneMatrixData> mCpuBoneMatrices;

    ComPtr<ID3D12Resource> mBoneMatrixBuffer;
    BoneMatrixData* mMappedBoneBuffer = nullptr;
    UINT mBoneMatrixSRVSlot = UINT_MAX;

    float mCurrentTime = 0.0f;
};