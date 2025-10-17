#include "PostFX_Util.hlsli"

float3 Heatmap(float value)
{
    value = saturate(value);
    float4 colors[5] =
    {
        float4(0, 0, 0, 0), // Black
        float4(0, 0, 1, 0.25f), // Blue
        float4(0, 1, 0, 0.5f), // Green
        float4(1, 1, 0, 0.75f), // Yellow
        float4(1, 0, 0, 1.0f) // Red
    };

    float3 finalColor = colors[0].rgb;
    for (int i = 1; i < 5; ++i)
    {
        finalColor = lerp(finalColor, colors[i].rgb, saturate((value - colors[i - 1].w) / (colors[i].w - colors[i - 1].w)));
    }
    return finalColor;
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
    float3 Normal = gGBuffer_Normal.Sample(gLinearSampler, input.uv).rgb;
    float3 Material = gGBuffer_Material.Sample(gLinearSampler, input.uv).rgb;
    float Roughness = Material.r;
    float Metallic = Material.g;
    float Depth = gDepthTex.Sample(gLinearSampler, input.uv).r;
    //=============================================
    float viewDepth = LinearizeDepth(Depth, gNearZ, gFarZ);
    uint clusterX = floor(input.uv.x * gClusterCountX);
    uint clusterY = floor(input.uv.y * gClusterCountY);
    float log_viewZ = log(viewDepth / gNearZ);
    float log_far_over_near = log(gFarZ / gNearZ);
    uint clusterZ = floor(saturate(log_viewZ / log_far_over_near) * gClusterCountZ);
    uint clusterIndex = clusterZ * (gClusterCountX * gClusterCountY) + clusterY * gClusterCountX + clusterX;
    //=============================================
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
    else if (gRenderFlags & RENDER_DEBUG_CLUSTER_AABB)
    {
        ClusterBound bound = ClusterListSRV[clusterIndex];
        float farPlaneZ = abs(bound.maxPoint.z);
        finalColor = saturate(farPlaneZ / gFarZ).xxx;

        float3 dir = normalize(bound.minPoint);
        finalColor = dir * 0.5f + 0.5f;
    }
    else if (gRenderFlags & RENDER_DEBUG_CLUSTER_ID)
    {
        finalColor = float3(
            (float) clusterX / gClusterCountX,
            (float) clusterY / gClusterCountY,
            (float) clusterZ / gClusterCountZ
        );
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
        
        
        for (uint i = 0; i < gLightCount; ++i)
        {
            float3 light_view_pos = LightInput[i].position;
            float4 light_clip_pos = mul(float4(light_view_pos, 1.0f), gProj);
            float3 light_ndc = light_clip_pos.xyz / light_clip_pos.w;

            float2 light_uv;
            light_uv.x = light_ndc.x * 0.5f + 0.5f;
            light_uv.y = -light_ndc.y * 0.5f + 0.5f;

            if (distance(input.uv, light_uv) < 0.005f)
            {
                finalColor = float3(1.0f, 0.0f, 0.0f); 
                break; 
            }
        }

        
        
    }
    else // RENDER_DEBUG_DEFAULT
    {
        float3 pixel_world_pos = ReconstructWorldPos(input.uv, Depth);
        float3 pixel_view_pos = mul(float4(pixel_world_pos, 1.0f), gView).xyz;
        float3 normal_view = normalize(mul(Normal, (float3x3) gView));
        
        float3 V = normalize(-pixel_view_pos);

        uint clusterX = floor(input.uv.x * gClusterCountX);
        uint clusterY = floor(input.uv.y * gClusterCountY);
        float viewDepth = length(pixel_view_pos);

        float log_viewZ = log(viewDepth / gNearZ);
        float log_far_over_near = log(gFarZ / gNearZ);
        uint clusterZ = floor(saturate(log_viewZ / log_far_over_near) * gClusterCountZ);
        uint clusterIndex = clusterZ * (gClusterCountX * gClusterCountY) + clusterY * gClusterCountX + clusterX;

        ClusterLightMeta meta = ClusterLightMetaSRV[clusterIndex];
        uint lightCount = meta.count;
        uint lightOffset = meta.offset;
        
        for (uint i = 0; i < lightCount; ++i)
        {
            uint lightIndex = ClusterLightIndicesSRV[lightOffset + i];
            LightInfo light = LightInput[lightIndex];

            float3 light_pos_view = light.position;
            
            float3 L = light_pos_view - pixel_view_pos;
            float distanceToLight = length(L);
            L = normalize(L);
            float3 H = normalize(L + V);

            float NdotL = saturate(dot(normal_view, L));
            float NdotV = saturate(dot(normal_view, V));
            float NdotH = saturate(dot(normal_view, H));
            float VdotH = saturate(dot(V, H));

            float attenuation = pow(saturate(1.0 - distanceToLight / light.range), 2.0);
            float3 radiance = light.color * light.intensity * attenuation;
            
            float3 F0 = lerp(0.04.xxx, Albedo, Metallic); 
            
            float D = D_GGX(NdotH, Roughness);
            float G = G_Smith(NdotV, NdotL, Roughness);
            float3 F = F_Schlick(VdotH, F0);
            
            float3 specular = (D * G * F) / (4.0 * NdotV * NdotL + 0.001);
            
            float3 kS = F;
            float3 kD = 1.0 - kS;
            kD *= (1.0 - Metallic); 
            
            float3 diffuse = kD * Albedo / PI;

            finalColor += (diffuse + specular) * radiance * NdotL;
        }

    }



    return float4(finalColor, 1.0f);
}
