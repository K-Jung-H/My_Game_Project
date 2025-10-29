#pragma once
#include "Core/Component.h"

constexpr UINT SPOT_SHADOW_RESOLUTION = 1024;
constexpr UINT CSM_SHADOW_RESOLUTION = 2048;
constexpr UINT POINT_SHADOW_RESOLUTION = 512;

constexpr UINT NUM_CSM_CASCADES = 4; // Number with 4N
constexpr UINT NUM_CUBE_FACES = 6;

constexpr UINT MAX_SHADOW_SPOT = 16;
constexpr UINT MAX_SHADOW_CSM = 4;
constexpr UINT MAX_SHADOW_POINT = 20;

constexpr UINT MAX_SHADOW_VIEWS = MAX_SHADOW_SPOT + (MAX_SHADOW_POINT * 6) + (MAX_SHADOW_CSM * NUM_CSM_CASCADES);

enum class DirectionalShadowMode : UINT
{
    DynamicCSM = 0,
    StaticGlobal = 1
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
    UINT padding;

    XMFLOAT4X4 LightViewProj[6];
    XMFLOAT4 cascadeSplits;
};


class TransformComponent;
class CameraComponent;
class Object;

class LightComponent : public Component
{
public:
    virtual rapidjson::Value ToJSON(rapidjson::Document::AllocatorType& alloc) const;
    virtual void FromJSON(const rapidjson::Value& val);

    static D3D12_VIEWPORT Get_ShadowMapViewport(Light_Type type);
    static D3D12_RECT Get_ShadowMapScissorRect(Light_Type type);

public:
    LightComponent();
    virtual ~LightComponent() = default;

    static constexpr Component_Type Type = Component_Type::Light;
    Component_Type GetType() const override { return Type; }

    void SetTransform(std::weak_ptr<TransformComponent> tf) { mTransform = tf; }
    std::shared_ptr<TransformComponent> GetTransform() { return mTransform.lock(); }

    const XMFLOAT4X4& UpdateShadowViewProj(std::shared_ptr<CameraComponent> mainCamera, UINT index = 0);
    XMMATRIX ComputeDirectionalViewProj(std::shared_ptr<CameraComponent> mainCamera, UINT cascadeIndex);

    const XMFLOAT4X4& GetShadowViewProj(UINT index) const;


    void SetPosition(const XMFLOAT3& pos);
    const XMFLOAT3& GetPosition();


    void SetLightType(Light_Type t) { lightType = t; mShadowMatrixDirty = true; }
    Light_Type GetLightType() const { return lightType; }

    void SetColor(const XMFLOAT3& color) { mColor = color; }
    const XMFLOAT3& GetColor() const { return mColor; }

    void SetDirection(const XMFLOAT3& dir);
    const XMFLOAT3& GetDirection() const { return mDirection; }

    void SetIntensity(float intensity) { mIntensity = intensity; }
    float GetIntensity() const { return mIntensity; }

    void SetRange(float range);
    float GetRange() const { return mRange; }

    void SetInnerAngle(float inner_angle) { mInnerAngle = inner_angle; mShadowMatrixDirty = true; }
    float GetInnerAngle() const { return mInnerAngle; }

    void SetOuterAngle(float out_angle) { mOuterAngle = out_angle; mShadowMatrixDirty = true; }
    float GetOuterAngle() const { return mOuterAngle; }

    void SetCastShadow(bool bUseShadow) { mCastsShadow = bUseShadow; }
    bool CastsShadow() const { return mCastsShadow; }

    void SetLightMask(UINT mask) { mLightMask = mask; }
    UINT GetLightMask() const { return mLightMask; }

    void SetVolumetricStrength(float strength) { mVolumetricStrength = strength; }
    float GetVolumetricStrength() const { return mVolumetricStrength; }

    void SetShadowMapNear(float nearZ);
    void SetShadowMapFar(float farZ);
    float GetShadowMapNear() const { return shadow_nearZ; }
    float GetShadowMapFar()  const { return shadow_farZ; }

	void SetCascadeLambda(float lambda) { cascadeLambda = lambda; }
	float GetCascadeLambda() const { return cascadeLambda; }

    //void SetShadowMode(DirectionalShadowMode mode);
    //DirectionalShadowMode GetShadowMode() const { return mShadowMode; }

    virtual void Update();

    GPULight ToGPUData() const;

protected:
    std::weak_ptr<TransformComponent> mTransform;
	Light_Type lightType = Light_Type::Point;

    XMFLOAT3 mPosition = { 0, 0, 0 };
    XMFLOAT3 mDirection = { 0, -1, 0 };


    XMFLOAT3 mColor = { 1.0f, 1.0f, 1.0f };
    float mIntensity = 10.0f;
    float mRange = 1000.0f;
    float mInnerAngle = XM_PIDIV4;
    float mOuterAngle = XM_PIDIV2;

    UINT mLightMask = 0xFFFFFFFF;
    float mVolumetricStrength = 1.0f;


    bool mCastsShadow = true;
    bool mShadowMatrixDirty = true;

	float shadow_nearZ = 0.1f;
	float shadow_farZ = 1000.0f;

    DirectionalShadowMode mShadowMode = DirectionalShadowMode::DynamicCSM;
    XMFLOAT4X4 mStaticShadowViewProj;

	float cascadeLambda = 0.5f;
    float cascadeSplits[NUM_CSM_CASCADES];

    bool mCsmMatrixDirty = true;
    


    XMFLOAT4X4 mCachedLightViewProj[NUM_CUBE_FACES];

};
