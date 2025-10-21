#include "LightComponent.h"
#include "TransformComponent.h"
#include "Core/Object.h"

LightComponent::LightComponent()
{
    XMFLOAT3 mColor = { 1.0f, 1.0f, 1.0f };
    float mIntensity = 10.0f;
    float mRange = 1000.0f;
    float mInnerAngle = XM_PIDIV4;
    float mOuterAngle = XM_PIDIV2;
    bool mCastsShadow = false;

    XMFLOAT3 mPosition = { 0, 0, 0 };
    XMFLOAT3 mDirection = { 0, -1, 0 };

    UINT mLightMask = 0xFFFFFFFF;
    float mVolumetricStrength = 1.0f;
}


rapidjson::Value LightComponent::ToJSON(rapidjson::Document::AllocatorType& alloc) const
{
    Value v(kObjectType);
    v.AddMember("type", "LightComponent", alloc);
    v.AddMember("LightType", static_cast<uint32_t>(lightType), alloc);

    Value pos(kArrayType);
    pos.PushBack(mPosition.x, alloc).PushBack(mPosition.y, alloc).PushBack(mPosition.z, alloc);
    v.AddMember("Position", pos, alloc);

    Value direction(kArrayType);
    direction.PushBack(mDirection.x, alloc).PushBack(mDirection.y, alloc).PushBack(mDirection.z, alloc);
    v.AddMember("Direction", direction, alloc);

    Value color(kArrayType);
    color.PushBack(mColor.x, alloc).PushBack(mColor.y, alloc).PushBack(mColor.z, alloc);
    v.AddMember("Color", color, alloc);

    v.AddMember("Intensity", mIntensity, alloc);
    v.AddMember("Range", mRange, alloc);
    v.AddMember("InnerAngle", mInnerAngle, alloc);
    v.AddMember("OuterAngle", mOuterAngle, alloc);
    v.AddMember("LightMask", mLightMask, alloc);
    v.AddMember("VolumetricStrength", mVolumetricStrength, alloc);
    v.AddMember("CastsShadow", mCastsShadow, alloc);

    return v;
}

void LightComponent::FromJSON(const rapidjson::Value& val)
{
    if (val.HasMember("LightType"))
        lightType = static_cast<Light_Type>(val["LightType"].GetUint());

    if (val.HasMember("Position") && val["Position"].IsArray())
    {
        const auto& pos = val["Position"].GetArray();
        if (pos.Size() == 3)
            mPosition = XMFLOAT3(pos[0].GetFloat(), pos[1].GetFloat(), pos[2].GetFloat());
    }

    if (val.HasMember("Direction") && val["Direction"].IsArray())
    {
        const auto& dir = val["Direction"].GetArray();
        if (dir.Size() == 3)
            mDirection = XMFLOAT3(dir[0].GetFloat(), dir[1].GetFloat(), dir[2].GetFloat());
    }

    if (val.HasMember("Color") && val["Color"].IsArray())
    {
        const auto& color = val["Color"].GetArray();
        if (color.Size() == 3)
            mColor = XMFLOAT3(color[0].GetFloat(), color[1].GetFloat(), color[2].GetFloat());
    }

    if (val.HasMember("Intensity"))           mIntensity = val["Intensity"].GetFloat();
    if (val.HasMember("Range"))               mRange = val["Range"].GetFloat();
    if (val.HasMember("InnerAngle"))          mInnerAngle = val["InnerAngle"].GetFloat();
    if (val.HasMember("OuterAngle"))          mOuterAngle = val["OuterAngle"].GetFloat();
    if (val.HasMember("VolumetricStrength"))  mVolumetricStrength = val["VolumetricStrength"].GetFloat();
    if (val.HasMember("CastsShadow"))         mCastsShadow = val["CastsShadow"].GetBool();
    if (val.HasMember("LightMask"))           mLightMask = val["LightMask"].GetUint();
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

const XMFLOAT3& LightComponent::GetPosition()
{
    if (auto tf = mTransform.lock())
        return tf->GetPosition();
    else
    {
        if (mOwner)
            mTransform = mOwner->GetTransform();

        if (auto tf = mTransform.lock())
            return tf->GetPosition();

        throw std::runtime_error("Failed to Find Light's Transform");
    }
}

void LightComponent::SetPosition(const XMFLOAT3& pos)
{
    if (auto tf = mTransform.lock())
        tf->SetPosition(pos);
    else
        throw std::runtime_error("Failed to Find Light's Transform");
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
    }
    else
    {
        if (mOwner)
            mTransform = mOwner->GetTransform();
        else
            throw std::runtime_error("Failed to Find Light's Transform");
    }
}


