#pragma once
#include "Core/Component.h"

constexpr UINT SPOT_SHADOW_RESOLUTION = 1024;
constexpr UINT CSM_SHADOW_RESOLUTION = 2048;
constexpr UINT POINT_SHADOW_RESOLUTION = 512;

constexpr UINT NUM_CSM_CASCADES = 4;
constexpr UINT NUM_CUBE_FACES = 6;

constexpr UINT MAX_SHADOW_SPOT = 16;
constexpr UINT MAX_SHADOW_CSM = 4;
constexpr UINT MAX_SHADOW_POINT = 20;

constexpr UINT MAX_SHADOW_VIEWS = MAX_SHADOW_SPOT + (MAX_SHADOW_POINT * 6) + (MAX_SHADOW_CSM * NUM_CSM_CASCADES);

enum class ShadowMode : UINT
{
    Dynamic = 0,
    Static = 1
};

enum class DirectionalShadowMode : UINT
{
    Default = 0, // 카메라 비의존적인 단일 섀도우
    CSM = 1      // 카메라 의존적인 4-Cascade 섀도우
};

enum class Light_Type
{
    Directional = 0,
    Point,
    Spot,
    etc,
};

struct GPULight
{
    XMFLOAT3 position; 
    float intensity;
    
    XMFLOAT3 direction; 
    UINT type;
    
    XMFLOAT3 color; 
    UINT castsShadow;

    float range;
    float spotOuterCosAngle;
    float spotInnerCosAngle;
    float volumetricStrength;

    UINT shadowMapStartIndex;
    UINT shadowMapLength;
    UINT lightMask;
    UINT directionalShadowMode;

    XMFLOAT4 cascadeSplits;
    
    float shadowNearZ;
    float shadowFarZ;
    XMFLOAT2 padding;
};


class TransformComponent;
class CameraComponent;
class Object;


class LightComponent : public Component
{
    static constexpr UINT FrameCount = Engine::Frame_Render_Buffer_Count;

public:
    // --- Component Interface ---
    LightComponent();
    virtual ~LightComponent() = default;
    static constexpr Component_Type Type = Component_Type::Light;
    virtual Component_Type GetType() const override;
    virtual void Update() override;

    // --- Serialization ---
    virtual rapidjson::Value ToJSON(rapidjson::Document::AllocatorType& alloc) const;
    virtual void FromJSON(const rapidjson::Value& val);

    // --- Static Utilities ---
    static D3D12_VIEWPORT Get_ShadowMapViewport(Light_Type type);
    static D3D12_RECT Get_ShadowMapScissorRect(Light_Type type);

    // --- Core Properties (Setters) ---
    void SetLightType(Light_Type t);
    void SetColor(const XMFLOAT3& color);
    void SetDirection(const XMFLOAT3& dir);
    void SetIntensity(float intensity);
    void SetRange(float range);
    void SetInnerAngle(float inner_angle);
    void SetOuterAngle(float out_angle);
    void SetLightMask(UINT mask);
    void SetVolumetricStrength(float strength);

    // --- Core Properties (Getters) ---
    Light_Type GetLightType() const;
    const XMFLOAT3& GetColor() const;
    const XMFLOAT3& GetDirection() const;
    float GetIntensity() const;
    float GetRange() const;
    float GetInnerAngle() const;
    float GetOuterAngle() const;
    UINT GetLightMask() const;
    float GetVolumetricStrength() const;

    // --- Shadow Properties (Setters) ---
    void SetCastShadow(bool bUseShadow);
    void SetShadowMode(ShadowMode mode);
    void SetDirectionalShadowMode(DirectionalShadowMode mode);
    void SetStaticOrthoSize(float size);
    void SetCascadeLambda(float lambda);
    void SetShadowMapNear(float nearZ);
    void SetShadowMapFar(float farZ);

    // --- Shadow Properties (Getters) ---
    bool CastsShadow() const;
    ShadowMode GetShadowMode() const;
    DirectionalShadowMode GetDirectionalShadowMode() const;
    float GetStaticOrthoSize() const;
    float GetCascadeLambda() const;
    float GetShadowMapNear() const;
    float GetShadowMapFar() const;

    // --- Renderer/Engine Interface ---
    GPULight ToGPUData() const;
    const XMFLOAT4X4& UpdateShadowViewProj(std::shared_ptr<CameraComponent> mainCamera, UINT index = 0);
    const XMFLOAT4X4& GetShadowViewProj(UINT index) const;
    void NotifyCameraMoved();
    bool NeedsShadowUpdate(UINT frameIndex) const;
    void ForceShadowMapUpdate();
    void ClearStaticBakeFlag(UINT frameIndex);

    // --- Transform Wrappers ---
    void SetTransform(std::weak_ptr<TransformComponent> tf);
    std::shared_ptr<TransformComponent> GetTransform();
    void SetPosition(const XMFLOAT3& pos);
    const XMFLOAT3& GetPosition();

protected:
    // --- Shadow Implementation ---
    void UpdateShadowMatrix_Directional(std::shared_ptr<CameraComponent> mainCamera, UINT index);
    void UpdateShadowMatrix_Spot();
    void UpdateShadowMatrix_Point();

protected:
    // --- Member Variables ---

    // Core
    std::weak_ptr<TransformComponent> mTransform;
    Light_Type lightType = Light_Type::Point;
    XMFLOAT3 mPosition = { 0, 0, 0 };
    XMFLOAT3 mDirection = { 0, -1, 0 };
    XMFLOAT3 mColor = { 1.0f, 1.0f, 1.0f };
    float mIntensity = 10.0f;
    float mRange = 1000.0f;
    float mLightMask = 0xFFFFFFFF;
    float mVolumetricStrength = 1.0f;

    // Spot
    float mInnerAngle = XM_PIDIV4;
    float mOuterAngle = XM_PIDIV2;

    // Shadow Core
    bool mCastsShadow = true;
    ShadowMode mShadowMode = ShadowMode::Dynamic;
    float shadow_nearZ = 0.1f;
    float shadow_farZ = 1000.0f;
    XMFLOAT4X4 mCachedLightViewProj[NUM_CUBE_FACES];

    // Shadow Flags
    bool mLightPropertiesDirty = true;
    bool mCsmProjectionDirty = true;
    bool mNeedsStaticBake[FrameCount] = { false };

    // Directional Shadow
    DirectionalShadowMode mDirectionalShadowMode = DirectionalShadowMode::CSM;
    float mStaticOrthoSize = 100.0f;
    float cascadeLambda = 0.5f;
    float cascadeSplits[NUM_CSM_CASCADES];
};