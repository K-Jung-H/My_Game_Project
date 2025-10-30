#include "PostFX_Util.hlsli"
#include "Lights.hlsli"

float3 GetCubeFaceDir(uint faceIndex, float2 localNDC)
{
    float3 dir = 0;

    switch (faceIndex)
    {
        case 0:
            dir = float3(1.0f, -localNDC.y, -localNDC.x);
            break;
        case 1:
            dir = float3(-1.0f, -localNDC.y, localNDC.x);
            break;

        case 2:
            dir = float3(localNDC.x, 1.0f, localNDC.y);
            break;


        case 3:
            dir = float3(localNDC.x, -1.0f, -localNDC.y);
            break;

        case 4:
            dir = float3(localNDC.x, -localNDC.y, 1.0f);
            break;

        case 5:
        default:
            dir = float3(-localNDC.x, -localNDC.y, -1.0f);
            break;
    }

    return normalize(dir);
}

float DebugPointShadowCubeFaces(float2 fullUV)
{
    float2 tileGrid = float2(3.0f, 2.0f);

    uint tileX = (uint) floor(fullUV.x * tileGrid.x); 
    uint tileY = (uint) floor(fullUV.y * tileGrid.y); 
    uint faceIndex = tileY * 3 + tileX; 

    float2 localUV = frac(fullUV * tileGrid);

    float2 localNDC = localUV * 2.0f - 1.0f;

    float3 dir = GetCubeFaceDir(faceIndex, localNDC);


    const uint cubeIndex = 0;


    float nearZ = 0.1f;
    float farZ = 1000.0f;
    float d = gShadowMapPoint.Sample(gLinearSampler, float4(dir, cubeIndex)).r;
    float linDepth = (nearZ * farZ) / (farZ - d * (farZ - nearZ));
    
    return (linDepth / farZ);
}

float4 DebugCSMShadowSlices(float2 fullUV)
{
    const uint cascadeCount = NUM_CSM_CASCADES;
    const uint lightIndex = 0;
    const uint sliceBase = lightIndex * cascadeCount;

    const float2 grid = float2(2.0f, 2.0f);

    uint tileX = (uint) floor(fullUV.x * grid.x);
    uint tileY = (uint) floor(fullUV.y * grid.y);

    uint cascadeIndex = tileY * 2 + tileX;
    if (cascadeIndex >= cascadeCount)
        return float4(0, 0, 0, 1); 

    float2 localUV = frac(fullUV * grid);

    uint sliceIndex = sliceBase + cascadeIndex;
    float depth = gShadowMapCSM.Sample(gLinearSampler, float3(localUV, sliceIndex)).r;
    depth = 1.0f - depth; // [Reverse-Z 보정]

    float3 tint[4] =
    {
        float3(1.0, 0.2, 0.2), // 0: near
        float3(0.2, 1.0, 0.2), // 1
        float3(0.2, 0.4, 1.0), // 2
        float3(1.0, 1.0, 0.2) // 3: far
    };


    float3 color = lerp(tint[cascadeIndex] * 0.5f, tint[cascadeIndex], depth);
    return float4(color, 1.0f);
}

float ComputeShadow(LightInfo light, float3 world_pos)
{
    // 기본값: 그림자 없음(밝음)
    float shadowFactor = 1.0f;

    // 그림자 비활성 시 바로 리턴
    if (light.castsShadow == 0)
        return 1.0f;

    float4 shadowPosH = 0;
    float2 shadowUV = 0;
    float pixelDepth = 0;
    uint sliceIndex = 0;

    switch (light.type)
    {
        // ───────────────────────────────────────────────
        // 0. Directional Light (CSM)
        // ───────────────────────────────────────────────
        case 0:
        {
                if (light.directionalShadowMode == 1) // 1 = CSM
                {
                    float4 viewPos = mul(float4(world_pos, 1.0f), gView);
                    float viewDepth = abs(viewPos.z);

                    uint cascadeIndex = 0;
                    [unroll]
                    for (uint i = 0; i < NUM_CSM_CASCADES - 1; ++i)
                    {
                        if (viewDepth > light.cascadeSplits[i])
                            cascadeIndex = i + 1;
                    }

                    shadowPosH = mul(float4(world_pos, 1.0f), light.LightViewProj[cascadeIndex]);
                    pixelDepth = shadowPosH.w;
                    shadowPosH.xyz /= shadowPosH.w;
                    pixelDepth = shadowPosH.z;
                    sliceIndex = (light.shadowMapStartIndex - gCsmShadowBaseOffset) + cascadeIndex;
                }
                else // 0 = Default (StaticGlobal)
                {
                    shadowPosH = mul(float4(world_pos, 1.0f), light.LightViewProj[0]);
                    pixelDepth = shadowPosH.w; 
                    shadowPosH.xyz /= shadowPosH.w;
                    pixelDepth = shadowPosH.z;
                    sliceIndex = (light.shadowMapStartIndex - gCsmShadowBaseOffset);
                }
            
                shadowUV = shadowPosH.xy * float2(0.5f, -0.5f) + 0.5f;
                shadowFactor = gShadowMapCSM.SampleCmpLevelZero(gShadowSampler, float3(shadowUV, sliceIndex), pixelDepth);

                if (any(shadowUV < 0.0f) || any(shadowUV > 1.0f))
                    shadowFactor = 1.0f;

                break;
            }


        // ───────────────────────────────────────────────
        // 1. Point Light (Omnidirectional, CubeMap)
        // ───────────────────────────────────────────────
        case 1:
        {
                break;
            }

        // ───────────────────────────────────────────────
        // 2. Spot Light (Perspective)
        // ───────────────────────────────────────────────
        case 2:
        {
                shadowPosH = mul(float4(world_pos, 1.0f), light.LightViewProj[0]);
                shadowPosH.xyz /= shadowPosH.w;

                shadowUV = shadowPosH.xy * float2(0.5f, -0.5f) + 0.5f;
                pixelDepth = shadowPosH.z;

                sliceIndex = light.shadowMapStartIndex - gSpotShadowBaseOffset;

                shadowFactor = gShadowMapSpot.SampleCmpLevelZero(
                gShadowSampler,
                float3(shadowUV, sliceIndex),
                pixelDepth
            );

            // 클리핑 처리
                if (any(shadowUV < 0.0f) || any(shadowUV > 1.0f))
                    shadowFactor = 1.0f;

                break;
            }
    }

    // ───────────────────────────────────────────────
    // Directional / Spot 은 UV 범위 체크 필수
    // Point 는 큐브맵 샘플이라 생략
    // ───────────────────────────────────────────────
    if (light.type != 1)
    {
        if (any(shadowUV < 0.0f) || any(shadowUV > 1.0f))
            shadowFactor = 1.0f;
    }

    return shadowFactor;
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
    // ────────────────────────────────────────────────
    float3 Normal = gGBuffer_Normal.Sample(gLinearSampler, input.uv).rgb;
    float3 world_normal = normalize(Normal * 2.0f - 1.0f);
    float3 view_normal = normalize(mul(world_normal, (float3x3) gView));
    // ────────────────────────────────────────────────
    float3 Material = gGBuffer_Material.Sample(gLinearSampler, input.uv).rgb;
    float Roughness = Material.r;
    float Metallic = Material.g;
    // ────────────────────────────────────────────────
    float Depth = gDepthTex.Sample(gLinearSampler, input.uv).r;
    float viewDepth = LinearizeDepth(Depth, gNearZ, gFarZ);
    // ────────────────────────────────────────────────
    uint clusterX = floor(input.uv.x * gClusterCountX);
    uint clusterY = floor(input.uv.y * gClusterCountY);
    float log_viewZ = log(viewDepth / gNearZ);
    float log_far_over_near = log(gFarZ / gNearZ);
    uint clusterZ = floor(saturate(log_viewZ / log_far_over_near) * gClusterCountZ);
    uint clusterIndex = clusterZ * (gClusterCountX * gClusterCountY) + clusterY * gClusterCountX + clusterX;
    // ────────────────────────────────────────────────

    
    float3 finalColor = float3(0.0, 0.0, 0.0);

    if (gRenderFlags & RENDER_DEBUG_ALBEDO)
    {
        return float4(Albedo, 1.0f);
    }
    else if (gRenderFlags & RENDER_DEBUG_NORMAL)
        finalColor = view_normal * 0.5f + 0.5f;
    else if (gRenderFlags & RENDER_DEBUG_ROUGHNESS)
        finalColor = Roughness.xxx;
    else if (gRenderFlags & RENDER_DEBUG_METALLIC)
        finalColor = Metallic.xxx;

    else if (gRenderFlags & RENDER_DEBUG_DEPTH_SCREEN)
    {
        finalColor = (1.0f - Depth).xxx;
    }
    else if (gRenderFlags & RENDER_DEBUG_DEPTH_VIEW)
    {
        finalColor = saturate((1.0f - viewDepth) / gFarZ).xxx;
    }
    else if (gRenderFlags & RENDER_DEBUG_DEPTH_WORLD)
    {
        float3 world_pos = ReconstructWorldPos(input.uv, Depth);
        finalColor = normalize(world_pos.xyz) * 0.5f + 0.5f;
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
        //float cubeDepthViz = DebugPointShadowCubeFaces(input.uv);
        //finalColor = cubeDepthViz.xxx;

        //float shadowDepth = gShadowMapSpot.Sample(gLinearSampler, float3(input.uv, 0.0f)).r;
        
        //shadowDepth = 1.0f - shadowDepth; // [Reverse-Z]
        //return float4(shadowDepth.xxx, 1.0f);
        
        float4 depthVis_color = DebugCSMShadowSlices(input.uv);
        finalColor = depthVis_color;
    }
    else
    { 
        float3 world_pos = ReconstructWorldPos(input.uv, Depth);
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
            
            float3 light_value = ComputeLight(light, view_pos, view_normal, V, Albedo, Metallic, Roughness);
            float shadow_factor = ComputeShadow(light, world_pos);
            
            finalColor += light_value * shadow_factor;
//            finalColor = shadow_factor.xxx;

        }
    }

    return float4(finalColor, 1.0f);
}
