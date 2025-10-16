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
        float viewDepth = LinearizeDepth(Depth, gNearZ, gFarZ);
        uint clusterX = floor(input.uv.x * gClusterCountX);
        uint clusterY = floor(input.uv.y * gClusterCountY);
        float log_viewZ = log(viewDepth / gNearZ);
        float log_far_over_near = log(gFarZ / gNearZ);
        uint clusterZ = floor((log_viewZ / log_far_over_near) * gClusterCountZ);
        uint clusterIndex = clusterZ * (gClusterCountX * gClusterCountY) + clusterY * gClusterCountX + clusterX;

        ClusterBound bound = ClusterListSRV[clusterIndex];


        float farPlaneZ = abs(bound.maxPoint.z); 
        finalColor = saturate(farPlaneZ / gFarZ).xxx;

        float3 posVec = normalize(bound.minPoint);
        finalColor = posVec * 0.5f + 0.5f;
    }
    else if (gRenderFlags & RENDER_DEBUG_CLUSTER_ID)
    {
        float viewDepth = LinearizeDepth(Depth, gNearZ, gFarZ);

        uint clusterX = floor(input.uv.x * gClusterCountX);
        uint clusterY = floor(input.uv.y * gClusterCountY);

        float log_viewZ = log(viewDepth / gNearZ);
        float log_far_over_near = log(gFarZ / gNearZ);
        uint clusterZ = floor((log_viewZ / log_far_over_near) * gClusterCountZ);

        finalColor = float3(
            (float) clusterX / gClusterCountX,
            (float) clusterY / gClusterCountY,
            (float) clusterZ / gClusterCountZ
        );
    }
    else if (gRenderFlags & RENDER_DEBUG_LIGHT_COUNT)
    {
        float viewDepth = LinearizeDepth(Depth, gNearZ, gFarZ);
        uint clusterX = floor(input.uv.x * gClusterCountX);
        uint clusterY = floor(input.uv.y * gClusterCountY);
        float log_viewZ = log(viewDepth / gNearZ);
        float log_far_over_near = log(gFarZ / gNearZ);
        uint clusterZ = floor((log_viewZ / log_far_over_near) * gClusterCountZ);
        
        uint clusterIndex = clusterZ * (gClusterCountX * gClusterCountY) + clusterY * gClusterCountX + clusterX;

        ClusterLightMeta meta = ClusterLightMetaSRV[clusterIndex];
        uint lightCount = meta.count;

        float normalizedCount = saturate((float) lightCount / 16.0f);
        finalColor = Heatmap(normalizedCount);
        // finalColor = normalizedCount.xxx;
    }
    else // RENDER_DEBUG_DEFAULT
    {
        finalColor = Albedo;
    }

    return float4(finalColor, 1.0f);
}
