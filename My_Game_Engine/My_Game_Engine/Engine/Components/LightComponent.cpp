#include "LightComponent.h"
#include "TransformComponent.h"

LightComponent::LightComponent()
{

}


rapidjson::Value LightComponent::ToJSON(rapidjson::Document::AllocatorType& alloc) const
{
    Value v(kObjectType);
    return v;
}

void LightComponent::FromJSON(const rapidjson::Value& val)
{
}


GPULight LightComponent::ToGPUData() const
{
    GPULight g{};

    g.position = mPosition;
    g.range = mRange;
    g.direction = mDirection;
    g.intensity = mIntensity;
    g.color = mColor;
    g.spotOuterCosAngle = cosf(mOuterAngle / 2.0f);
    g.spotInnerCosAngle = cosf(mInnerAngle / 2.0f);
    g.type = static_cast<UINT>(lightType);
    g.castsShadow = mCastsShadow ? 1u : 0u;
    g.lightMask = mLightMask;
    g.volumetricStrength = mVolumetricStrength;
    g.shadowMapStartIndex = mShadowMapStartIndex;
    g.shadowMapLength = mShadowMapLength;

    return g;
}

void LightComponent::SetDirection(const XMFLOAT3& dir)
{
    XMVECTOR v = XMVector3Normalize(XMLoadFloat3(&dir));
    XMStoreFloat3(&mDirection, v);
}

void LightComponent::Update()
{
    if (auto tf = mTransform.lock())
    {
		mPosition = tf->GetPosition();
        //mDirection = tf->GetLook();
    }
}


