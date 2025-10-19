#include "PostFX_Util.hlsli"
#include "Lights.hlsli"

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

float3 ComputeLight(
    LightInfo light,
    float3 view_pos,
    float3 view_normal,
    float3 V,
    float3 Albedo,
    float Metallic,
    float Roughness)
{
    float3 L = 0;
    float3 radiance = 0;
    float3 result = 0;

    switch (light.type)
    {
        case 0:
        {
                L = normalize(-light.direction);
                radiance = light.color * light.intensity;
                result = PBR_Lighting(L, V, view_normal, radiance, Albedo, Metallic, Roughness);
                break;
            }
        case 1:
        {
                float3 toLight = light.position - view_pos;
                float dist = length(toLight);
                L = normalize(toLight);
                float attenuation = pow(saturate(1.0 - dist / light.range), 2.0);
                radiance = light.color * light.intensity * attenuation;
                result = PBR_Lighting(L, V, view_normal, radiance, Albedo, Metallic, Roughness);
                break;
            }
        case 2:
        {
                float3 toLight = light.position - view_pos;
                float dist = length(toLight);
                L = normalize(toLight);
                float attenuation = pow(saturate(1.0 - dist / light.range), 2.0);
                float3 lightDir = normalize(-light.direction);
                float cosTheta = dot(L, lightDir);
                float spotFalloff = saturate((cosTheta - light.spotOuterCosAngle) / (light.spotInnerCosAngle - light.spotOuterCosAngle + 0.0001f));
                radiance = light.color * light.intensity * attenuation * spotFalloff;
                result = PBR_Lighting(L, V, view_normal, radiance, Albedo, Metallic, Roughness);
                break;
            }
        default:
            break;
    }

    return result;
}

VS_SCREEN_OUT Default_VS(uint nVertexID : SV_VertexID)
{
    VS_SCREEN_OUT output;
    output = FullscreenQuad_VS(nVertexID);
    return output;
}

float4 Default_PS(VS_SCREEN_OUT input) : SV_TARGET
{
    float3 Albedo = gGBuffer_Albedo.Sample(gLinearSampler, input.uv).rgb;
    // 式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式
    float3 Normal = gGBuffer_Normal.Sample(gLinearSampler, input.uv).rgb;
    float3 world_normal = normalize(Normal * 2.0f - 1.0f);
    float3 view_normal = normalize(mul(world_normal, (float3x3) gView));
    // 式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式
    float3 Material = gGBuffer_Material.Sample(gLinearSampler, input.uv).rgb;
    float Roughness = Material.r;
    float Metallic = Material.g;
    // 式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式
    float Depth = gDepthTex.Sample(gLinearSampler, input.uv).r;
    float viewDepth = LinearizeDepth(Depth, gNearZ, gFarZ);
    // 式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式
    uint clusterX = floor(input.uv.x * gClusterCountX);
    uint clusterY = floor(input.uv.y * gClusterCountY);
    float log_viewZ = log(viewDepth / gNearZ);
    float log_far_over_near = log(gFarZ / gNearZ);
    uint clusterZ = floor(saturate(log_viewZ / log_far_over_near) * gClusterCountZ);
    uint clusterIndex = clusterZ * (gClusterCountX * gClusterCountY) + clusterY * gClusterCountX + clusterX;
    // 式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式式

    
    float3 finalColor = float3(0.0, 0.0, 0.0);

    if (gRenderFlags & RENDER_DEBUG_ALBEDO)
    {
        float3 world_pos = ReconstructWorldPos(input.uv, viewDepth);
        float3 dir = normalize(world_pos);
        float3 color = dir * 0.5f + 0.5f;
        return float4(color, 1.0f);
    }
    else if (gRenderFlags & RENDER_DEBUG_NORMAL)
        finalColor = view_normal * 0.5f + 0.5f;
    else if (gRenderFlags & RENDER_DEBUG_ROUGHNESS)
        finalColor = Roughness.xxx;
    else if (gRenderFlags & RENDER_DEBUG_METALLIC)
        finalColor = Metallic.xxx;
    else if (gRenderFlags & RENDER_DEBUG_DEPTH_VIEW)
    {
        float vDepth = LinearizeDepth(Depth, gNearZ, gFarZ);
        finalColor = saturate(vDepth / gFarZ).xxx;
    }
    else if (gRenderFlags & RENDER_DEBUG_CLUSTER_ID)
    {
        finalColor = float3(
            (float) clusterX / gClusterCountX,
            (float) clusterY / gClusterCountY,
            (float) clusterZ / gClusterCountZ);
    }
    else if (gRenderFlags & RENDER_DEBUG_LIGHT_COUNT)
    {
        ClusterLightMeta meta = ClusterLightMetaSRV[clusterIndex];
        uint lightCount = meta.count;
        uint offset = meta.offset;
        float normalizedCount = saturate((float) lightCount / 16.0f);
        float3 heatColor = Heatmap(normalizedCount);
        float3 metaColor = float3(frac(offset * 0.0001f), normalizedCount, (lightCount > 0) ? 1.0f : 0.0f);
        finalColor = lerp(metaColor, heatColor, 0.6f);
    }
    else
    {
        float3 world_pos = ReconstructWorldPos(input.uv, viewDepth);
        float3 view_pos = mul(float4(world_pos, 1.0f), gView).xyz;
        float3 V = normalize(-view_pos);

        ClusterLightMeta meta = ClusterLightMetaSRV[clusterIndex];
        uint lightCount = meta.count;
        uint lightOffset = meta.offset;

        [loop]
        for (uint i = 0; i < lightCount; ++i)
        {
            uint lightIndex = ClusterLightIndicesSRV[lightOffset + i];
            LightInfo light = LightInput[lightIndex];
            finalColor += ComputeLight(light, view_pos, view_normal, V, Albedo, 0, 1);
        }
    }

    return float4(finalColor, 1.0f);
}
