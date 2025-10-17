#include "Core/Component.h"

enum class LightType
{
    Directional = 0,
    Point,
    Spot,
    etc,
};

struct GPULight
{
    XMFLOAT3 position;
    float range;

    XMFLOAT3 direction;
    float intensity;

    XMFLOAT3 color;
    UINT type;

    bool castsShadow;
    UINT shadowMapStartIndex;
    UINT shadowMapLength;
    UINT padding0;

};

class TransformComponent;

class LightComponent : public Component
{
public:
    virtual rapidjson::Value ToJSON(rapidjson::Document::AllocatorType& alloc) const;
    virtual void FromJSON(const rapidjson::Value& val);

public:
    LightComponent();
    virtual ~LightComponent() = default;

    static constexpr Component_Type Type = Component_Type::Light;
    Component_Type GetType() const override { return Type; }

    void SetTransform(std::weak_ptr<TransformComponent> tf) { mTransform = tf; }
    std::shared_ptr<TransformComponent> GetTransform() { return mTransform.lock(); }

    void SetLightType(LightType t) { lightType = t; }
    LightType GetLightType() const { return lightType; }

    void SetColor(const XMFLOAT3& color) { mColor = color; }
    const XMFLOAT3& GetColor() const { return mColor; }

    void SetIntensity(float intensity) { mIntensity = intensity; }
    float GetIntensity() const { return mIntensity; }

    void SetRange(float range) { mRange = range; }
    float GetRange() const { return mRange; }

    void SetInnerAngle(float inner_angle) { mInnerAngle = inner_angle; }
    float GetInnerAngle() const { return mInnerAngle; }

    void SetOuterAngle(float out_angle) { mOuterAngle = out_angle; }
    float GetOuterAngle() const { return mOuterAngle; }

    void SetCastShadow(bool bUseShadow) { mCastsShadow = bUseShadow; }
    bool CastsShadow() const { return mCastsShadow; }

    virtual void Update();

    GPULight ToGPUData() const;

protected:
    std::weak_ptr<TransformComponent> mTransform;
	LightType lightType = LightType::Point;

    XMFLOAT3 mColor = { 1.0f, 1.0f, 1.0f };
    float mIntensity = 10.0f;
    float mRange = 1000.0f;
    float mInnerAngle = XM_PIDIV4;
    float mOuterAngle = XM_PIDIV2;
    bool mCastsShadow = false;

    XMFLOAT3 mPosition = { 0, 0, 0 };
    XMFLOAT3 mDirection = { 0, -1, 0 };
};
