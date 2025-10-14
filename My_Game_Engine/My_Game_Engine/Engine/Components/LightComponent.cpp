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
    g.type = static_cast<uint32_t>(lightType);
    g.castsShadow = mCastsShadow ? 1u : 0u;
    g.shadowMapStartIndex = UINT_MAX;
    g.shadowMapLength = 0;
    g.padding0 = 0;
    return g;
}


void LightComponent::Update()
{
    if (auto tf = mTransform.lock())
    {
		mPosition = tf->GetPosition();
        //mDirection = tf->GetLook();
    }
}


