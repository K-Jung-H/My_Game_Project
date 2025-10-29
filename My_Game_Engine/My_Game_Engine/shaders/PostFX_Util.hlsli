
#define RENDER_DEBUG_DEFAULT   (1 << 0)
#define RENDER_DEBUG_ALBEDO    (1 << 1)
#define RENDER_DEBUG_NORMAL    (1 << 2)
#define RENDER_DEBUG_ROUGHNESS (1 << 3)
#define RENDER_DEBUG_METALLIC  (1 << 4)
#define RENDER_DEBUG_DEPTH_SCREEN (1 << 5)
#define RENDER_DEBUG_DEPTH_VIEW (1 << 6)
#define RENDER_DEBUG_DEPTH_WORLD (1 << 7)
#define RENDER_DEBUG_CLUSTER_AABB (1 << 8)
#define RENDER_DEBUG_CLUSTER_ID       (1 << 9) 
#define RENDER_DEBUG_LIGHT_COUNT      (1 << 10) 

static const float PI = 3.14159265359;

cbuffer SceneCB : register(b0)
{
    float4 gTimeInfo;
    
    uint gLightCount;
    uint gClusterIndexCapacity;
    uint gRenderFlags;
    float padding;
};

cbuffer CameraCB : register(b1)
{
    float4x4 gView;
    float4x4 gProj;
    float4x4 gInvView;
    float4x4 gInvProj;
    float4x4 gInvViewProj;
    float3 gCameraPos;
    float gPadding0;

    float gNearZ;
    float gFarZ;
    float2 gPadding1;

    uint gClusterCountX;
    uint gClusterCountY;
    uint gClusterCountZ;
    float gPadding2;
};

// === GBuffer Textures ===
// enum class GBufferType : UINT
// {
//     Albedo = 0,  // t0
//     Normal,      // t1
//     Material,    // t2
//     Count
// };

struct ClusterBound
{
    float3 minPoint;
    float pad0;
    float3 maxPoint;
    float pad1;
};

static const uint NUM_CUBE_FACES = 6;
static const uint MAX_SHADOW_SPOT = 16;
static const uint MAX_SHADOW_CSM = 4;
static const uint MAX_SHADOW_POINT = 20;
static const uint NUM_CSM_CASCADES = 4;

static const uint gPointShadowBaseOffset = 0;
static const uint gCsmShadowBaseOffset = gPointShadowBaseOffset + (MAX_SHADOW_POINT* NUM_CUBE_FACES);
static const uint gSpotShadowBaseOffset = gCsmShadowBaseOffset + (MAX_SHADOW_CSM* NUM_CSM_CASCADES);

struct LightInfo
{
    float3 position;
    float intensity;

    float3 direction;
    uint type;

    float3 color;
    uint castsShadow;

    float range;
    float spotOuterCosAngle;
    float spotInnerCosAngle;
    float volumetricStrength;

    uint shadowMapStartIndex;
    uint shadowMapLength;
    uint lightMask;
    uint padding;

    float4x4 LightViewProj[6];
    float4 cascadeSplits;

};


struct ClusterLightMeta
{
    uint offset;
    uint count;
    float2 padding0;
};

Texture2D gGBuffer_Albedo : register(t0);
Texture2D gGBuffer_Normal : register(t1);
Texture2D gGBuffer_Material : register(t2);
Texture2D gDepthTex : register(t3);
Texture2D gMergeTex : register(t4);

StructuredBuffer<ClusterBound> ClusterListSRV : register(t5);
StructuredBuffer<LightInfo> LightInput : register(t6);
StructuredBuffer<ClusterLightMeta> ClusterLightMetaSRV : register(t7);
StructuredBuffer<uint> ClusterLightIndicesSRV : register(t8);

Texture2DArray gShadowMapCSM : register(t9);
TextureCubeArray gShadowMapPoint : register(t10);
Texture2DArray gShadowMapSpot : register(t11);

SamplerState gLinearSampler : register(s0);
SamplerState gClampSampler : register(s1);
SamplerComparisonState gShadowSampler : register(s2);

struct VS_SCREEN_OUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

static const VS_SCREEN_OUT g_FullscreenQuadCorners[4] =
{
    { float4(-1.0, 1.0, 0.0, 1.0), float2(0.0, 0.0) }, // top-left
    { float4(1.0, 1.0, 0.0, 1.0), float2(1.0, 0.0) }, // top-right
    { float4(1.0, -1.0, 0.0, 1.0), float2(1.0, 1.0) }, // bottom-right
    { float4(-1.0, -1.0, 0.0, 1.0), float2(0.0, 1.0) } // bottom-left
};

VS_SCREEN_OUT FullscreenQuad_VS(uint vid : SV_VertexID)
{
    static const uint indices[6] = { 0, 1, 2, 0, 2, 3 };

    return g_FullscreenQuadCorners[indices[vid]];
}

// === 선형화 함수 (View-space Depth 복원) ===
float LinearizeDepth(float depth, float nearZ, float farZ)
{
    return (nearZ * farZ) / (depth * (farZ - nearZ) + nearZ);
}


float3 ReconstructWorldPos(float2 uv, float depth)
{
    float2 ndc = float2(uv.x * 2.0f - 1.0f, (1.0f - uv.y) * 2.0f - 1.0f);

    float4 clipPos = float4(ndc, depth, 1.0f);

    float4 viewPos = mul(clipPos, gInvProj);
    viewPos /= viewPos.w;
    float4 worldPos = mul(viewPos, gInvView);
    return worldPos.xyz;
}

float3 Heatmap(float value)
{
    value = saturate(value);
    float4 colors[5] =
    {
        float4(0, 0, 0, 0),
        float4(0, 0, 1, 0.25f),
        float4(0, 1, 0, 0.5f),
        float4(1, 1, 0, 0.75f),
        float4(1, 0, 0, 1.0f)
    };

    float3 finalColor = colors[0].rgb;
    for (int i = 1; i < 5; ++i)
    {
        finalColor = lerp(finalColor, colors[i].rgb, saturate((value - colors[i - 1].w) / (colors[i].w - colors[i - 1].w)));
    }
    return finalColor;
}