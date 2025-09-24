
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
};

struct VS_DEFAULT_OUTPUT
{
    float4 position : SV_POSITION;
};


VS_DEFAULT_OUTPUT Default_VS(VS_DEFAULT_INPUT input)
{
    VS_DEFAULT_OUTPUT output;

    output.position = float4(input.position, 1.0f);

    return output;
}

float4 Default_PS(VS_DEFAULT_OUTPUT input) : SV_TARGET
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}