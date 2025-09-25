#include "PostFX_Util.hlsli"


VS_SCREEN_OUT Default_VS(uint nVertexID : SV_VertexID)
{
    VS_SCREEN_OUT output;
    output = FullscreenQuad_VS(nVertexID);
  
    return output;
}

float4 Default_PS(VS_SCREEN_OUT input) : SV_TARGET
{
    float3 colorTexture = gGBuffer_Albedo.Sample(gLinearSampler, input.uv);
    return float4(colorTexture, 1.0f);
}