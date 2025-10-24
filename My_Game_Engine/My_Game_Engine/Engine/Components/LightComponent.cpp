#include "LightComponent.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "Core/Object.h"

D3D12_VIEWPORT LightComponent::Get_ShadowMapViewport(Light_Type type)
{
    D3D12_VIEWPORT viewport = {};
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    switch (type)
    {
    case Light_Type::Point:
        viewport.Width = (float)POINT_SHADOW_RESOLUTION;
        viewport.Height = (float)POINT_SHADOW_RESOLUTION;
        break;
    case Light_Type::Directional:
        viewport.Width = (float)CSM_SHADOW_RESOLUTION;
        viewport.Height = (float)CSM_SHADOW_RESOLUTION;
        break;
    case Light_Type::Spot:
        viewport.Width = (float)SPOT_SHADOW_RESOLUTION;
        viewport.Height = (float)SPOT_SHADOW_RESOLUTION;
        break;
    }
    return viewport;
}

D3D12_RECT LightComponent::Get_ShadowMapScissorRect(Light_Type type)
{
    D3D12_RECT scissorRect = {};
    scissorRect.left = 0;
    scissorRect.top = 0;

    switch (type)
    {
    case Light_Type::Point:
        scissorRect.right = POINT_SHADOW_RESOLUTION;
        scissorRect.bottom = POINT_SHADOW_RESOLUTION;
        break;
    case Light_Type::Directional:
        scissorRect.right = CSM_SHADOW_RESOLUTION;
        scissorRect.bottom = CSM_SHADOW_RESOLUTION;
        break;
    case Light_Type::Spot:
        scissorRect.right = SPOT_SHADOW_RESOLUTION;
        scissorRect.bottom = SPOT_SHADOW_RESOLUTION;
        break;
    }
    return scissorRect;
}


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
    g.shadowMapStartIndex = Engine::INVALID_ID;
    g.shadowMapLength = Engine::INVALID_ID;

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

    mShadowMatrixDirty = true;
}


void LightComponent::SetDirection(const XMFLOAT3& dir)
{
    XMVECTOR v = XMVector3Normalize(XMLoadFloat3(&dir));
    XMStoreFloat3(&mDirection, v);

    mShadowMatrixDirty = true;
}

void LightComponent::Update()
{
    if (auto tf = mTransform.lock())
    {
		mPosition = tf->GetPosition();
        if (tf->GetUpdateFlag())
        {
            mShadowMatrixDirty = true;
            mCsmMatrixDirty = true;
        }
    }
    else
    {
        if (mOwner)
            mTransform = mOwner->GetTransform();
        else
            throw std::runtime_error("Failed to Find Light's Transform");
    }
}

const XMFLOAT4X4& LightComponent::GetShadowViewProj(std::shared_ptr<CameraComponent> mainCamera, UINT index)
{
    if (lightType != Light_Type::Directional && !mShadowMatrixDirty)
    {
        return mCachedLightViewProj[index];
    }

    if (lightType == Light_Type::Directional && !mCsmMatrixDirty)
    {
        return mCachedLightViewProj[index];
    }

    if (lightType == Light_Type::Spot)
    {
        XMVECTOR pos = XMLoadFloat3(&mPosition);
        XMVECTOR dir = XMLoadFloat3(&mDirection);
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        if (abs(XMVectorGetY(dir)) > 0.99f)
        {
            up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); 
        }
        XMMATRIX view = XMMatrixLookToLH(pos, dir, up);
        XMMATRIX proj = XMMatrixPerspectiveFovLH(mOuterAngle, 1.0f, 0.1f, mRange);
        XMStoreFloat4x4(&mCachedLightViewProj[0], XMMatrixTranspose(view * proj));

        mShadowMatrixDirty = false;
    }
    else if (lightType == Light_Type::Point)
    {
        XMVECTOR pos = XMLoadFloat3(&mPosition);
        XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, 0.1f, mRange);

        XMVECTOR targets[] = {
            XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),
            XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f),
            XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
            XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f),
            XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
            XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f)
        };
        XMVECTOR ups[] = {
            XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
            XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
            XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f),
            XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
            XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
            XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
        };

        for (UINT i = 0; i < 6; ++i)
        {
            XMMATRIX view = XMMatrixLookToLH(pos, targets[i], ups[i]);
            XMMATRIX viewProj = view * proj;
            XMStoreFloat4x4(&mCachedLightViewProj[i], XMMatrixTranspose(viewProj));
        }

        mShadowMatrixDirty = false;
    }
    else if (lightType == Light_Type::Directional)
    {
        if (mainCamera && index < NUM_CSM_CASCADES)
        {
            XMMATRIX viewProj = XMMatrixIdentity();
            XMStoreFloat4x4(&mCachedLightViewProj[index], XMMatrixTranspose(viewProj));
        }
        if (index == 4 - 1)
            mCsmMatrixDirty = false;
    }

    return mCachedLightViewProj[index];
}