
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

float2 pos[3] = { float2(-1.0f, -1.0f), float2(-1.0f, 3.0f), float2(3.0f, -1.0f) };

float2 uv[3] = { float2(0.0f, 1.0f), float2(0.0f, -1.0f), float2(2.0f, 1.0f) };

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VS_OUT Default_VS(uint vertexID : SV_VertexID)
{
    VS_OUT output;

    output.position = float4(pos[vertexID], 0.0f, 1.0f);
    output.uv = uv[vertexID];

    return output;
}

float4 Default_PS(VS_OUT input) : SV_TARGET
{
    return float4(input.uv, 0.0f, 1.0f);
}