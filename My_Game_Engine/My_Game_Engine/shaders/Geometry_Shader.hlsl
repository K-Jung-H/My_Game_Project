
cbuffer SceneConstants : register(b0)
{
    float4 gTimeInfo;
};


cbuffer ObjectCB : register(b1)
{
    float4x4 gWorld;
};

cbuffer CameraCB : register(b2)
{
    float4x4 gView;
    float4x4 gProj;
    float3 CameraPos;
    float Padding;
};

Texture2D gDiffuseTex : register(t0);
Texture2D gNormalTex : register(t1);
Texture2D gRoughnessTex : register(t2);
Texture2D gMetallicTex : register(t3);

SamplerState gLinearSampler : register(s0);
SamplerState gClampSampler : register(s1);


struct VS_DEFAULT_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv0 : TEXCOORD;
    float4 color : COLOR;
};

struct VS_DEFAULT_OUTPUT
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float4 color : COLOR0;
};

VS_DEFAULT_OUTPUT Default_VS(VS_DEFAULT_INPUT input)
{
    VS_DEFAULT_OUTPUT output;

    float4 worldPos = mul(float4(input.position, 1.0f), gWorld);
    output.worldPos = worldPos.xyz;

    float4 viewPos = mul(worldPos, gView);
    output.position = mul(viewPos, gProj);

    output.normal = mul(float4(input.normal, 0.0f), gWorld).xyz;
    output.uv = input.uv0;
    output.color = input.color;

    return output;
}

float4 Default_PS(VS_DEFAULT_OUTPUT input) : SV_TARGET
{
    float4 albedo = gDiffuseTex.Sample(gLinearSampler, input.uv);
    return albedo * input.color;
}
