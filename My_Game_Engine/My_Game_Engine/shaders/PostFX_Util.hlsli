
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

#define NUM_CUBE_FACES 6

struct LightInfo
{
    float3 position;
    float range;

    float3 direction;
    float intensity;

    float3 color;
    uint type;

    float spotOuterCosAngle;
    float spotInnerCosAngle;
    uint castsShadow;
    uint lightMask;
    
    float volumetricStrength;
    uint shadowMapStartIndex;
    uint shadowMapLength;
    uint padding;
    
    float4x4 LightViewProj[NUM_CUBE_FACES];
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
    float z = depth * 2.0f - 1.0f; // [0,1] → [-1,1]
    return (2.0f * nearZ * farZ) / (farZ + nearZ - z * (farZ - nearZ));
}

float3 ReconstructWorldPos(float2 uv, float linearViewZ)
{
    float2 ndcXY = uv * 2.0f - 1.0f;
    ndcXY.y = -ndcXY.y;
    float viewSpaceX = ndcXY.x * gInvProj._11 * linearViewZ;
    float viewSpaceY = ndcXY.y * gInvProj._22 * linearViewZ;

    float4 viewSpacePos = float4(viewSpaceX, viewSpaceY, linearViewZ, 1.0f);

    float4 worldPos = mul(viewSpacePos, gInvView);

    return worldPos.xyz / worldPos.w;
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