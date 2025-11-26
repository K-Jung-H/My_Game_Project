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


cbuffer ObjectCB : register(b2)
{
    float4x4 gWorld;
    float4 Albedo;
    float Roughness;
    float Metallic;
    float Emissive;
    int DiffuseTexIdx;
    int NormalTexIdx;
    int RoughnessTexIdx;
    int MetallicTexIdx;
};



Texture2D gTextures[2000] : register(t0);
SamplerState gLinearSampler : register(s0);

struct VS_IN
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float4 color0 : COLOR;
};

struct VS_OUT
{
    float4 svPosition : SV_POSITION;
    float3 worldNormal : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

VS_OUT Default_VS(VS_IN i)
{
    VS_OUT o;
    float4 wpos = mul(float4(i.pos, 1.0f), gWorld);
    float4 vpos = mul(wpos, gView);
    o.svPosition = mul(vpos, gProj);

    o.worldNormal = normalize(mul(float4(i.normal, 0.0f), gWorld).xyz);
    o.uv = i.uv0;
    return o;
}

struct PS_OUT
{
    float4 Albedo : SV_Target0;
    float4 Normal : SV_Target1;
    float4 Material : SV_Target2;
};

float4 SampleIfValid(int texIdx, float2 uv)
{
    if (texIdx < 0)
        return float4(1, 1, 1, 1);
    return gTextures[texIdx].Sample(gLinearSampler, uv);
}

PS_OUT Default_PS(VS_OUT i)
{
    PS_OUT o;

    float4 texAlbedo = SampleIfValid(DiffuseTexIdx, i.uv);
    o.Albedo = texAlbedo * Albedo;

    float3 n = normalize(i.worldNormal);
    o.Normal = float4(0.5f * (n + 1.0f), 1.0f);

    float rough = Roughness;
    if (RoughnessTexIdx >= 0)
        rough *= SampleIfValid(RoughnessTexIdx, i.uv).r;

    float metal = Metallic;
    if (MetallicTexIdx >= 0)
        metal *= SampleIfValid(MetallicTexIdx, i.uv).r;

    float emis = Emissive;

    o.Material = float4(saturate(rough), saturate(metal), emis, 1.0f);
    return o;
}