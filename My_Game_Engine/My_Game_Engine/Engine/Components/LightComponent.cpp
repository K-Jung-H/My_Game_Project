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
    : lightType(Light_Type::Point)
    , mPosition(0.0f, 0.0f, 0.0f)
    , mDirection(0.0f, -1.0f, 0.0f)
    , mColor(1.0f, 1.0f, 1.0f)
    , mIntensity(10.0f)
    , mRange(1000.0f)
    , mInnerAngle(XM_PIDIV4)
    , mOuterAngle(XM_PIDIV2)
    , mLightMask(0xFFFFFFFF)
    , mVolumetricStrength(1.0f)
    , mCastsShadow(true)
    , shadow_nearZ(10.0f)
    , shadow_farZ(1000.0f)
    , mShadowMatrixDirty(true)
    , mCsmMatrixDirty(true)
{
    for (UINT i = 0; i < NUM_CUBE_FACES; ++i)
    {
        XMStoreFloat4x4(&mCachedLightViewProj[i], XMMatrixIdentity());
    }
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
    g.direction = mDirection;
    g.intensity = mIntensity;
    g.color = mColor;
    g.type = static_cast<UINT>(lightType);
    g.castsShadow = mCastsShadow ? 1u : 0u;

    g.range = mRange;
    g.spotOuterCosAngle = cosf(mOuterAngle * 0.5f);
    g.spotInnerCosAngle = cosf(mInnerAngle * 0.5f);
    g.volumetricStrength = mVolumetricStrength;

    g.shadowMapStartIndex = Engine::INVALID_ID;
    g.shadowMapLength = Engine::INVALID_ID;
    g.lightMask = mLightMask;
    g.padding = 0;

    for (int i = 0; i < 6; ++i)
        g.LightViewProj[i] = mCachedLightViewProj[i];

    g.cascadeSplits = XMFLOAT4(cascadeSplits[0], cascadeSplits[1], cascadeSplits[2], cascadeSplits[3]);

    
    return g;
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

const XMFLOAT4X4& LightComponent::UpdateShadowViewProj(std::shared_ptr<CameraComponent> mainCamera, UINT index)
{
    if (lightType != Light_Type::Directional && !mShadowMatrixDirty)
        return mCachedLightViewProj[index];

    //if (lightType == Light_Type::Directional && !mCsmMatrixDirty)
    //    return mCachedLightViewProj[index];

    if (lightType == Light_Type::Directional)
    {
        XMMATRIX lightViewProj = ComputeDirectionalViewProj(mainCamera, index);

        XMStoreFloat4x4(&mCachedLightViewProj[index], XMMatrixTranspose(lightViewProj));

        if (index == NUM_CSM_CASCADES - 1)
            mCsmMatrixDirty = false;

        return mCachedLightViewProj[index];
    }
    else if (lightType == Light_Type::Spot)
    {
        XMVECTOR pos = XMLoadFloat3(&mPosition);
        XMVECTOR dir = XMLoadFloat3(&mDirection);
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        if (abs(XMVectorGetY(dir)) > 0.99f)
            up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

        XMMATRIX view = XMMatrixLookToLH(pos, dir, up);
        XMMATRIX proj = XMMatrixPerspectiveFovLH(mOuterAngle, 1.0f, shadow_farZ, shadow_nearZ);

        XMStoreFloat4x4(&mCachedLightViewProj[0], XMMatrixTranspose(view * proj));
        mShadowMatrixDirty = false;
    }
    else if (lightType == Light_Type::Point)
    {
        const float nearZ = shadow_nearZ;
        float farZ = shadow_farZ;

        static const XMVECTORF32 dirs[6] = {
            {  1,  0,  0, 0 },
            { -1,  0,  0, 0 },
            {  0,  1,  0, 0 },
            {  0, -1,  0, 0 },
            {  0,  0,  1, 0 },
            {  0,  0, -1, 0 }
        };

        static const XMVECTORF32 ups[6] = {
            { 0, 1, 0, 0 },
            { 0, 1, 0, 0 },
            { 0, 0,-1, 0 },
            { 0, 0, 1, 0 },
            { 0, 1, 0, 0 },
            { 0, 1, 0, 0 }
        };

        XMVECTOR eye = XMLoadFloat3(&mPosition);
        XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, farZ, nearZ);

        for (int face = 0; face < 6; ++face)
        {
            XMMATRIX view = XMMatrixLookToLH(eye, dirs[face], ups[face]);
            XMMATRIX vp = XMMatrixTranspose(view * proj);
            XMStoreFloat4x4(&mCachedLightViewProj[face], vp);
        }

        mShadowMatrixDirty = false;
        return mCachedLightViewProj[index];
    }


    return mCachedLightViewProj[index];
}

XMMATRIX LightComponent::ComputeDirectionalViewProj(std::shared_ptr<CameraComponent> mainCamera, UINT cascadeIndex)
{
    float nearZ = mainCamera->GetNearZ();
    float farZ = mainCamera->GetFarZ();
    float fovY = mainCamera->GetFovY();
    float aspect = mainCamera->GetAspectRatio();

    float lambda = cascadeLambda;
    float splits[NUM_CSM_CASCADES + 1];
    splits[0] = nearZ;

    for (UINT i = 0; i < NUM_CSM_CASCADES; ++i)
    {
        float p = (i + 1) / float(NUM_CSM_CASCADES);
        float logSplit = nearZ * powf(farZ / nearZ, p);
        float uniformSplit = nearZ + (farZ - nearZ) * p;
        splits[i + 1] = logSplit * lambda + uniformSplit * (1.0f - lambda);
    }

    float cascadeNear = splits[cascadeIndex];
    float cascadeFar = splits[cascadeIndex + 1];

    for (UINT i = 0; i < NUM_CSM_CASCADES; ++i)
        cascadeSplits[i] = splits[i + 1];

    XMMATRIX camView = mainCamera->GetViewMatrix();
	XMMATRIX camProj = XMMatrixPerspectiveFovLH(fovY, aspect, cascadeFar, cascadeNear); // Reverse Z
    XMMATRIX invCamViewProj = XMMatrixInverse(nullptr, camView * camProj);

    XMVECTOR ndcCorners[8] =
    {
        { -1,  1, 0, 1 }, { 1,  1, 0, 1 },
        {  1, -1, 0, 1 }, { -1, -1, 0, 1 },
        { -1,  1, 1, 1 }, { 1,  1, 1, 1 },
        {  1, -1, 1, 1 }, { -1, -1, 1, 1 }
    };

    XMVECTOR frustumCornersWS[8];
    for (int i = 0; i < 8; ++i)
    {
        XMVECTOR corner = XMVector4Transform(ndcCorners[i], invCamViewProj);
        frustumCornersWS[i] = corner / XMVectorSplatW(corner);
    }

    XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat3(&mDirection));
    XMVECTOR up = fabsf(XMVectorGetY(lightDir)) > 0.99f
        ? XMVectorSet(0, 0, 1, 0)
        : XMVectorSet(0, 1, 0, 0);

    XMVECTOR center = XMVectorZero();

    for (int i = 0; i < 8; ++i)
        center += frustumCornersWS[i];
    center /= 8.0f;

    XMVECTOR lightPos = center - lightDir * 500.0f;
    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, center, up);

    XMVECTOR minPt = XMVectorSet(+FLT_MAX, +FLT_MAX, +FLT_MAX, 0);
    XMVECTOR maxPt = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0);

    for (int i = 0; i < 8; ++i)
    {
        XMVECTOR cornerLS = XMVector3TransformCoord(frustumCornersWS[i], lightView);
        minPt = XMVectorMin(minPt, cornerLS);
        maxPt = XMVectorMax(maxPt, cornerLS);
    }


    float padX = (XMVectorGetX(maxPt) - XMVectorGetX(minPt)) * 0.05f;
    float padY = (XMVectorGetY(maxPt) - XMVectorGetY(minPt)) * 0.05f;
    float padZ = (XMVectorGetZ(maxPt) - XMVectorGetZ(minPt)) * 0.1f;
    minPt -= XMVectorSet(padX, padY, padZ, 0);
    maxPt += XMVectorSet(padX, padY, padZ, 0);

    float minX = XMVectorGetX(minPt);
    float maxX = XMVectorGetX(maxPt);
    float minY = XMVectorGetY(minPt);
    float maxY = XMVectorGetY(maxPt);
    float minZ = XMVectorGetZ(minPt);
    float maxZ = XMVectorGetZ(maxPt);


    XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(minX, maxX, minY, maxY, maxZ, minZ);


    return lightView * lightProj;
}

const XMFLOAT4X4& LightComponent::GetShadowViewProj(UINT index) const
{
    assert(index < NUM_CSM_CASCADES);
    return mCachedLightViewProj[index];
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

void LightComponent::SetShadowMapNear(float nearZ)
{
    nearZ = std::max(0.01f, nearZ);
    if (nearZ >= shadow_farZ - 0.1f)
        nearZ = shadow_farZ * 0.1f; 
    shadow_nearZ = nearZ;
    mShadowMatrixDirty = true;
}

void LightComponent::SetShadowMapFar(float farZ)
{
    farZ = std::max(farZ, shadow_nearZ + 1.0f);
    if (farZ / shadow_nearZ > 10000.0f)
        farZ = shadow_nearZ * 10000.0f; 
    shadow_farZ = farZ;
    mShadowMatrixDirty = true;
}

void LightComponent::SetRange(float range)
{
    mRange = std::max(1.0f, range);
    mShadowMatrixDirty = true;
}
