#include "LightComponent.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "Core/Object.h"

// --- Static Utilities ---

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

// --- Component Interface ---

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
    , mLightPropertiesDirty(true)
{
    for (UINT i = 0; i < NUM_CUBE_FACES; ++i)
    {
        XMStoreFloat4x4(&mCachedLightViewProj[i], XMMatrixIdentity());
    }
}

Component_Type LightComponent::GetType() const
{
    return Type;
}

void LightComponent::Update()
{
    if (auto tf = mTransform.lock())
    {
        mPosition = tf->GetPosition();
        if (tf->GetUpdateFlag())
        {
            mLightPropertiesDirty = true;
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

// --- Serialization ---

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

    v.AddMember("ShadowMode", static_cast<uint32_t>(mShadowMode), alloc);
    v.AddMember("DirectionalShadowMode", static_cast<uint32_t>(mDirectionalShadowMode), alloc);
    v.AddMember("ShadowNearZ", shadow_nearZ, alloc);
    v.AddMember("ShadowFarZ", shadow_farZ, alloc);
    v.AddMember("CascadeLambda", cascadeLambda, alloc);
    v.AddMember("StaticOrthoSize", mStaticOrthoSize, alloc);

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

    if (val.HasMember("ShadowMode"))
        mShadowMode = static_cast<ShadowMode>(val["ShadowMode"].GetUint());
    if (val.HasMember("DirectionalShadowMode"))
        mDirectionalShadowMode = static_cast<DirectionalShadowMode>(val["DirectionalShadowMode"].GetUint());
    if (val.HasMember("ShadowNearZ"))
        shadow_nearZ = val["ShadowNearZ"].GetFloat();
    if (val.HasMember("ShadowFarZ"))
        shadow_farZ = val["ShadowFarZ"].GetFloat();
    if (val.HasMember("CascadeLambda"))
        cascadeLambda = val["CascadeLambda"].GetFloat();
    if (val.HasMember("StaticOrthoSize"))
        mStaticOrthoSize = val["StaticOrthoSize"].GetFloat();
}

// --- Core Properties (Setters) ---
void LightComponent::SetLightType(Light_Type t)
{
    lightType = t;
    ForceShadowMapUpdate();
}

void LightComponent::SetColor(const XMFLOAT3& color)
{
    mColor = color;
}

void LightComponent::SetDirection(const XMFLOAT3& dir)
{
    XMVECTOR v = XMVector3Normalize(XMLoadFloat3(&dir));
    XMStoreFloat3(&mDirection, v);
    ForceShadowMapUpdate();
}

void LightComponent::SetIntensity(float intensity)
{
    mIntensity = intensity;
}

void LightComponent::SetRange(float range)
{
    mRange = std::max(1.0f, range);
    ForceShadowMapUpdate();
}

void LightComponent::SetInnerAngle(float inner_angle)
{
    mInnerAngle = inner_angle;
    ForceShadowMapUpdate();
}

void LightComponent::SetOuterAngle(float out_angle)
{
    mOuterAngle = out_angle;
    ForceShadowMapUpdate();
}

void LightComponent::SetLightMask(UINT mask)
{
    mLightMask = mask;
    ForceShadowMapUpdate();
}

void LightComponent::SetVolumetricStrength(float strength)
{
    mVolumetricStrength = strength;
}

// --- Core Properties (Getters) ---
Light_Type LightComponent::GetLightType() const
{
    return lightType;
}

const XMFLOAT3& LightComponent::GetColor() const
{
    return mColor;
}

const XMFLOAT3& LightComponent::GetDirection() const
{
    return mDirection;
}

float LightComponent::GetIntensity() const
{
    return mIntensity;
}

float LightComponent::GetRange() const
{
    return mRange;
}

float LightComponent::GetInnerAngle() const
{
    return mInnerAngle;
}

float LightComponent::GetOuterAngle() const
{
    return mOuterAngle;
}

UINT LightComponent::GetLightMask() const
{
    return mLightMask;
}

float LightComponent::GetVolumetricStrength() const
{
    return mVolumetricStrength;
}

// --- Shadow Properties (Setters) ---
void LightComponent::SetCastShadow(bool bUseShadow)
{
    mCastsShadow = bUseShadow;
}

void LightComponent::SetShadowMode(ShadowMode mode)
{
    mShadowMode = mode;
    ForceShadowMapUpdate();
}

void LightComponent::SetDirectionalShadowMode(DirectionalShadowMode mode)
{
    mDirectionalShadowMode = mode;
    ForceShadowMapUpdate();
}

void LightComponent::SetStaticOrthoSize(float size)
{
    mStaticOrthoSize = size;
    ForceShadowMapUpdate();
}

void LightComponent::SetCascadeLambda(float lambda)
{
    cascadeLambda = lambda;
    ForceShadowMapUpdate();
}

void LightComponent::SetShadowMapNear(float nearZ)
{
    nearZ = std::max(0.01f, nearZ);
    if (nearZ >= shadow_farZ - 0.1f)
        nearZ = shadow_farZ * 0.1f;
    shadow_nearZ = nearZ;
    ForceShadowMapUpdate();
}

void LightComponent::SetShadowMapFar(float farZ)
{
    farZ = std::max(farZ, shadow_nearZ + 1.0f);
    if (farZ / shadow_nearZ > 10000.0f)
        farZ = shadow_nearZ * 10000.0f;
    shadow_farZ = farZ;
    ForceShadowMapUpdate();
}

// --- Shadow Properties (Getters) ---
bool LightComponent::CastsShadow() const
{
    return mCastsShadow;
}

ShadowMode LightComponent::GetShadowMode() const
{
    return mShadowMode;
}

DirectionalShadowMode LightComponent::GetDirectionalShadowMode() const
{
    return mDirectionalShadowMode;
}

float LightComponent::GetStaticOrthoSize() const
{
    return mStaticOrthoSize;
}

float LightComponent::GetCascadeLambda() const
{
    return cascadeLambda;
}

float LightComponent::GetShadowMapNear() const
{
    return shadow_nearZ;
}

float LightComponent::GetShadowMapFar() const
{
    return shadow_farZ;
}

// --- Renderer/Engine Interface ---

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
    g.directionalShadowMode = (lightType == Light_Type::Directional) ? static_cast<UINT>(mDirectionalShadowMode) : 0;

    g.cascadeSplits = XMFLOAT4(cascadeSplits[0], cascadeSplits[1], cascadeSplits[2], cascadeSplits[3]);

    g.shadowNearZ = shadow_nearZ;
    g.shadowFarZ = shadow_farZ;
    g.padding = XMFLOAT2(0.0f, 0.0f);

    return g;
}

const XMFLOAT4X4& LightComponent::UpdateShadowViewProj(std::shared_ptr<CameraComponent> mainCamera, UINT index)
{
    if (mShadowMode == ShadowMode::Static)
    {
        if (lightType == Light_Type::Directional)
        {
            if (mDirectionalShadowMode == DirectionalShadowMode::CSM)
            {
                if (!mLightPropertiesDirty && !mCsmProjectionDirty)
                    return mCachedLightViewProj[index];
            }
            else
            {
                if (!mLightPropertiesDirty)
                    return mCachedLightViewProj[index];
            }
        }
        else
        {
            if (!mLightPropertiesDirty)
                return mCachedLightViewProj[index];
        }
    }

    switch (lightType)
    {
    case Light_Type::Directional:
        UpdateShadowMatrix_Directional(mainCamera, index);
        break;
    case Light_Type::Spot:
        UpdateShadowMatrix_Spot();
        break;
    case Light_Type::Point:
        UpdateShadowMatrix_Point();
        break;
    }

    return mCachedLightViewProj[index];
}

const XMFLOAT4X4& LightComponent::GetShadowViewProj(UINT index) const
{
    assert(index < NUM_CUBE_FACES); // CSM은 4개지만 배열 크기는 6개이므로 6을 기준으로 함
    return mCachedLightViewProj[index];
}

void LightComponent::NotifyCameraMoved()
{
    if (lightType == Light_Type::Directional && mDirectionalShadowMode == DirectionalShadowMode::CSM)
        mCsmProjectionDirty = true;
}

bool LightComponent::NeedsShadowUpdate(UINT frameIndex) const
{
    if (mShadowMode == ShadowMode::Dynamic)
        return true;

    if (mNeedsStaticBake[frameIndex])
        return true;

    if (lightType == Light_Type::Directional)
    {
        if (mDirectionalShadowMode == DirectionalShadowMode::CSM)
            return mLightPropertiesDirty || mCsmProjectionDirty;
        else
            return mLightPropertiesDirty;
    }
    else
    {
        return mLightPropertiesDirty;
    }
}

void LightComponent::ForceShadowMapUpdate()
{
    mLightPropertiesDirty = true;
    mCsmProjectionDirty = true;

    for (int i = 0; i < FrameCount; ++i)
        mNeedsStaticBake[i] = true;
}

void LightComponent::ClearStaticBakeFlag(UINT frameIndex)
{
    mNeedsStaticBake[frameIndex] = false;
}

// --- Transform Wrappers ---

void LightComponent::SetTransform(std::weak_ptr<TransformComponent> tf)
{
    mTransform = tf;
}

std::shared_ptr<TransformComponent> LightComponent::GetTransform()
{
    return mTransform.lock();
}

void LightComponent::SetPosition(const XMFLOAT3& pos)
{
    if (auto tf = mTransform.lock())
        tf->SetPosition(pos);
    else
        throw std::runtime_error("Failed to Find Light's Transform");

    ForceShadowMapUpdate();
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

// --- Shadow Implementation ---

void LightComponent::UpdateShadowMatrix_Directional(std::shared_ptr<CameraComponent> mainCamera, UINT cascadeIndex)
{
    if (mDirectionalShadowMode == DirectionalShadowMode::CSM)
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
        XMMATRIX camProj = XMMatrixPerspectiveFovLH(fovY, aspect, cascadeFar, cascadeNear);
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

        float maxDistSq = 0.0f;
        for (int i = 0; i < 8; ++i)
        {
            float distSq = XMVectorGetX(XMVector3LengthSq(frustumCornersWS[i] - center));
            maxDistSq = std::max(maxDistSq, distSq);
        }
        float radius = sqrtf(maxDistSq);
        float worldSpaceSize = radius * 2.0f;

        XMVECTOR centerLS = XMVector3TransformCoord(center, lightView);

        XMVECTOR minPt = XMVectorSet(+FLT_MAX, +FLT_MAX, +FLT_MAX, 0);
        XMVECTOR maxPt = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0);
        for (int i = 0; i < 8; ++i)
        {
            XMVECTOR cornerLS = XMVector3TransformCoord(frustumCornersWS[i], lightView);
            minPt = XMVectorMin(minPt, cornerLS);
            maxPt = XMVectorMax(maxPt, cornerLS);
        }

        float minX = XMVectorGetX(centerLS) - radius;
        float minY = XMVectorGetY(centerLS) - radius;
        float maxX = XMVectorGetX(centerLS) + radius;
        float maxY = XMVectorGetY(centerLS) + radius;

        float padZ = (XMVectorGetZ(maxPt) - XMVectorGetZ(minPt)) * 0.1f;
        float minZ = XMVectorGetZ(minPt) - padZ;
        float maxZ = XMVectorGetZ(maxPt) + padZ;

        float texelSize = worldSpaceSize / (float)CSM_SHADOW_RESOLUTION;

        minX = floor(minX / texelSize) * texelSize;
        minY = floor(minY / texelSize) * texelSize;

        maxX = minX + worldSpaceSize;
        maxY = minY + worldSpaceSize;

        XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(minX, maxX, minY, maxY, maxZ, minZ);

        XMMATRIX lightViewProj = lightView * lightProj;
        XMStoreFloat4x4(&mCachedLightViewProj[cascadeIndex], XMMatrixTranspose(lightViewProj));

        if (cascadeIndex == NUM_CSM_CASCADES - 1)
        {
            mLightPropertiesDirty = false;
            mCsmProjectionDirty = false;
        }
    }
    else // Default (StaticGlobal)
    {
        if (cascadeIndex > 0)
            return;

        XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat3(&mDirection));
        XMVECTOR up = fabsf(XMVectorGetY(lightDir)) > 0.99f ? XMVectorSet(0, 0, 1, 0) : XMVectorSet(0, 1, 0, 0);

        XMVECTOR lightPos = XMVectorSet(0, 0, 0, 0) - lightDir * (shadow_farZ * 0.5f);
        XMMATRIX view = XMMatrixLookToLH(lightPos, lightDir, up);

        float halfSize = mStaticOrthoSize * 0.5f;
        XMMATRIX proj = XMMatrixOrthographicOffCenterLH(-halfSize, halfSize, -halfSize, halfSize, shadow_farZ, shadow_nearZ);

        XMStoreFloat4x4(&mCachedLightViewProj[0], XMMatrixTranspose(view * proj));
        mLightPropertiesDirty = false;
        mCsmProjectionDirty = false;
    }
}

void LightComponent::UpdateShadowMatrix_Spot()
{
    XMVECTOR pos = XMLoadFloat3(&mPosition);
    XMVECTOR dir = XMLoadFloat3(&mDirection);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    if (abs(XMVectorGetY(dir)) > 0.99f)
        up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    XMMATRIX view = XMMatrixLookToLH(pos, dir, up);
    XMMATRIX proj = XMMatrixPerspectiveFovLH(mOuterAngle, 1.0f, shadow_farZ, shadow_nearZ);

    XMStoreFloat4x4(&mCachedLightViewProj[0], XMMatrixTranspose(view * proj));
    mLightPropertiesDirty = false;
}

void LightComponent::UpdateShadowMatrix_Point()
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

    mLightPropertiesDirty = false;
}