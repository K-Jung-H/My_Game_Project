#include "PostFX_Util.hlsli"

VS_SCREEN_OUT Default_VS(uint nVertexID : SV_VertexID)
{
    VS_SCREEN_OUT output;
    output = FullscreenQuad_VS(nVertexID);
  
    return output;
}

float4 Default_PS(VS_SCREEN_OUT input) : SV_TARGET
{
    float3 Albedo = gGBuffer_Albedo.Sample(gLinearSampler, input.uv).rgb;
    float3 Normal = gGBuffer_Normal.Sample(gLinearSampler, input.uv).rgb;
    float3 Material = gGBuffer_Material.Sample(gLinearSampler, input.uv).rgb;
    float Roughness = Material.r;
    float Metallic = Material.g;
    float Depth = gDepthTex.Sample(gLinearSampler, input.uv).r;

    float3 finalColor = float3(0.0, 0.0, 0.0);


    if (gRenderFlags & RENDER_DEBUG_ALBEDO)
    {
        finalColor = Albedo;
    }
    else if (gRenderFlags & RENDER_DEBUG_NORMAL)
    {
        finalColor = Normal * 0.5f + 0.5f;
    }
    else if (gRenderFlags & RENDER_DEBUG_ROUGHNESS)
    {
        finalColor = Roughness.xxx;
    }
    else if (gRenderFlags & RENDER_DEBUG_METALLIC)
    {
        finalColor = Metallic.xxx;
    }
    else if (gRenderFlags & RENDER_DEBUG_DEPTH_SCREEN)
    {
        finalColor = Depth.xxx;
    }
    else if (gRenderFlags & RENDER_DEBUG_DEPTH_VIEW)
    {
        float viewDepth = LinearizeDepth(Depth, gNearZ, gFarZ);
        viewDepth = saturate(viewDepth / gFarZ);
        finalColor = viewDepth.xxx;
    }
    else if (gRenderFlags & RENDER_DEBUG_DEPTH_WORLD)
    {
        float3 worldPos = ReconstructWorldPos(input.uv, Depth);
        float dist = distance(gCameraPos, worldPos);
        dist = saturate(dist / gFarZ);
        finalColor = dist.xxx;
    }
    else // RENDER_DEBUG_DEFAULT
    {
        finalColor = Albedo;
    }

    return float4(finalColor, 1.0f);
}
