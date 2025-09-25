cbuffer SceneCB : register(b0)
{
    float4 gTimeInfo;
};

cbuffer CameraCB : register(b1)
{
    float4x4 gView;
    float4x4 gProj;
    float3 gCameraPos;
    float Padding;
};

// === GBuffer Textures ===
// enum class GBufferType : UINT
// {
//     Albedo = 0,  // t0
//     Normal,      // t1
//     Material,    // t2
//     Count
// };

Texture2D gGBuffer_Albedo : register(t0);
Texture2D gGBuffer_Normal : register(t1);
Texture2D gGBuffer_Material : register(t2);
Texture2D gDepthTex : register(t3);

Texture2D gMergeTex : register(t4);

SamplerState gLinearSampler : register(s0);
SamplerState gClampSampler : register(s1);


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
