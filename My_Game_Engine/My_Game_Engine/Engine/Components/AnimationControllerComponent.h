#pragma once
#include "Core/Component.h"
#include "Resource/Skeleton.h"
#include "Resource/Model_Avatar.h"
#include "Resource/AnimationLayer.h"

class TransformComponent;

struct ControllerBoneCache
{
    bool isRootMotion = false;
    bool hasMapping = false;
    int logicalParentIdx = -1;

    XMFLOAT3 bindScale;
    XMFLOAT4 bindRotation;
    XMFLOAT3 bindTranslation;

    XMMATRIX globalTransform;
};
struct BoneMatrixData
{
    XMFLOAT4X4 transform;
};

class AnimationControllerComponent : public SynchronizedComponent
{
public:
    virtual rapidjson::Value ToJSON(rapidjson::Document::AllocatorType& alloc) const;
    virtual void FromJSON(const rapidjson::Value& val);

public:
    static constexpr Component_Type Type = Component_Type::AnimationController;
    Component_Type GetType() const override { return Type; }

public:
    AnimationControllerComponent();
    virtual ~AnimationControllerComponent() = default;

    virtual void WakeUp();

    void SetTransform(std::weak_ptr<TransformComponent> tf) { mTransform = tf; }
    std::shared_ptr<TransformComponent> GetTransform() { return mTransform.lock(); }

    void SetSkeleton(std::shared_ptr<Skeleton> skeleton);
    void SetModelAvatar(std::shared_ptr<Model_Avatar> model_avatar);

    std::shared_ptr<Skeleton> GetSkeleton() { return mModelSkeleton; }
    std::shared_ptr<Model_Avatar> GetModelAvatar() { return mModelAvatar; }

    UINT GetBoneMatrixSRV() const { return mBoneMatrixSRVSlot; }

    void SetLayerCount(int count);
    int GetLayerCount() const { return (int)mLayers.size(); }

    void SetLayerWeight(int layerIndex, float weight);
    float GetLayerWeight(int layerIndex) const;

    void SetLayerMask(int layerIndex, std::shared_ptr<AvatarMask> mask);
    std::shared_ptr<AvatarMask> GetLayerMask(int layerIndex) const;

    void SetLayerNormalizedTime(int layerIndex, float ratio);
    float GetLayerNormalizedTime(int layerIndex) const;
    float GetLayerDuration(int layerIndex) const;

    void SetPlaybackMode(PlaybackMode mode, int layerIndex = 0);
    PlaybackMode GetPlaybackMode(int layerIndex = 0) const;

    void SetSpeed(float speed, int layerIndex = 0);
    float GetSpeed(int layerIndex = 0) const;

    bool IsLayerTransitioning(int layerIndex) const;
    float GetLayerTransitionProgress(int layerIndex) const;
    std::shared_ptr<AnimationClip> GetCurrentClip(int layerIndex) const;


    void SetPause(bool pause) { mIsPaused = pause; }
    bool IsPaused() const { return mIsPaused; }

    void Play(int layerIndex, std::shared_ptr<AnimationClip> clip, float blendTime = 0.2f, PlaybackMode mode = PlaybackMode::Loop, float speed = 1.0f);

    bool IsReady() const;
    void Update(float deltaTime);

private:
    void CreateBoneMatrixBuffer();
    void UpdateBoneMappingCache();
    void EvaluateLayers();


private:
    std::weak_ptr<TransformComponent> mTransform;

    std::shared_ptr<Model_Avatar> mModelAvatar;
    std::shared_ptr<Skeleton> mModelSkeleton;
    std::vector<AnimationLayer> mLayers;
    bool mIsPaused = false;

    std::vector<std::string> mCachedBoneToKey;
    std::vector<ControllerBoneCache> mControllerBoneCache;


    //-------------------------------------------------------
    std::vector<BoneMatrixData> mCpuBoneMatrices;
    ComPtr<ID3D12Resource> mBoneMatrixBuffer;
    BoneMatrixData* mMappedBoneBuffer = nullptr;
    UINT mBoneMatrixSRVSlot = UINT_MAX;
};