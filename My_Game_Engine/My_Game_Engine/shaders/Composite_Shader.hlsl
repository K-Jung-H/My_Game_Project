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

float3 GetCubeMapVector(float3 L_world, out uint faceIndex, out uint sliceOffset)
{
    float3 absL = abs(L_world);
    float maxComp = max(absL.x, max(absL.y, absL.z));
    float3 projVec = 0;
    faceIndex = 0;
    sliceOffset = 0;

    if (maxComp == absL.x)
    {
        if (L_world.x > 0)
        {
            faceIndex = 0;
            sliceOffset = 0;
            projVec = float3(-L_world.z, -L_world.y, L_world.x);
        }
        else
        {
            faceIndex = 1;
            sliceOffset = 1;
            projVec = float3(L_world.z, -L_world.y, -L_world.x);
        }
    }
    else if (maxComp == absL.y)
    {
        if (L_world.y > 0)
        {
            faceIndex = 2;
            sliceOffset = 2;
            projVec = float3(L_world.x, L_world.z, L_world.y);
        }
        else
        {
            faceIndex = 3;
            sliceOffset = 3;
            projVec = float3(L_world.x, -L_world.z, -L_world.y);
        }
    }
    else // Z major axis
    {
        if (L_world.z > 0)
        {
            faceIndex = 4;
            sliceOffset = 4;
            projVec = float3(L_world.x, -L_world.y, L_world.z);
        }
        else
        {
            faceIndex = 5;
            sliceOffset = 5;
            projVec = float3(-L_world.x, -L_world.y, -L_world.z);
        }
    }
    return projVec / maxComp;
}


float ComputeShadow(LightInfo light, float3 world_pos)
{
    float shadowFactor = 1.0f;

    if (light.castsShadow == 0)
    {
        return 1.0f;
    }

    float4 shadowPosH = float4(0, 0, 0, 0);
    float2 shadowUV = float2(0, 0);
    float pixelDepth = 0;
    uint sliceIndex = 0;

    switch (light.type)
    {
        case 0: // Directional (CSM)
        {
                uint cascadeIndex = 0;

                shadowPosH = mul(float4(world_pos, 1.0f), light.LightViewProj[cascadeIndex]);
                shadowPosH.xyz /= shadowPosH.w;

                shadowUV = shadowPosH.xy * float2(0.5f, -0.5f) + 0.5f;
                pixelDepth = shadowPosH.z;
                sliceIndex = light.shadowMapStartIndex + cascadeIndex;

                shadowFactor = gShadowMapCSM.SampleCmpLevelZero(gShadowSampler, float3(shadowUV, sliceIndex), pixelDepth);
                break;
            }
        case 1: // Point
        {
                float3 light_pos_world = mul(float4(light.position, 1.0f), gInvView).xyz;
                float3 lightToPixelDir = world_pos - light_pos_world;

                float nearZ = 0.1f;
                float farZ = light.range;
            
                float dist = max(abs(lightToPixelDir.x), max(abs(lightToPixelDir.y), abs(lightToPixelDir.z)));
                float pixelDepth = (farZ * (dist - nearZ)) / (dist * (farZ - nearZ));
                pixelDepth = saturate(pixelDepth);

                float bias = 0.001f;
                uint cubeIndex = (light.shadowMapStartIndex - gPointShadowBaseOffset) / 6;
            
                shadowFactor = gShadowMapPoint.SampleCmpLevelZero(gShadowSampler, float4(lightToPixelDir, cubeIndex), pixelDepth - bias);
                
                break;
            }
        case 2: // Spot
        {
                shadowPosH = mul(float4(world_pos, 1.0f), light.LightViewProj[0]);
                shadowPosH.xyz /= shadowPosH.w;

                shadowUV = shadowPosH.xy * float2(0.5f, -0.5f) + 0.5f;
                pixelDepth = shadowPosH.z;
                
                float bias = 0.001f;
                sliceIndex = light.shadowMapStartIndex - gSpotShadowBaseOffset;

                shadowFactor = gShadowMapSpot.SampleCmpLevelZero(gShadowSampler, float3(shadowUV, sliceIndex), pixelDepth - bias);
                break;
            }
    }

    if (light.type != 1)
    {
        if (any(shadowUV < 0.0f) || any(shadowUV > 1.0f))
        {
            shadowFactor = 1.0f;
        }
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
        return float4(Albedo, 1.0f);
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
        float cubeDepthViz = DebugPointShadowCubeFaces(input.uv);
        finalColor = cubeDepthViz.xxx;

        //float shadowDepth = gShadowMapCSM.Sample(gLinearSampler, float3(input.uv, 0.0f)).r;
        //float shadowDepth = gShadowMapSpot.Sample(gLinearSampler, float3(input.uv, 0.0f)).r;
        //float d = gShadowMapSpot.Sample(gLinearSampler, float3(input.uv, 0)).r;
        
        //float nearZ = 0.1f;
        //float farZ = 1000.0f;
        //float shadowDepth = gShadowMapSpot.Sample(gLinearSampler, float3(input.uv, 0.0f)).r;
        //float linDepth = (nearZ * farZ) / (farZ - shadowDepth * (farZ - nearZ));
    
        //return float4((linDepth / farZ).xxx, 1.0f);
       
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
            
            float3 light_value = ComputeLight(light, view_pos, view_normal, V, Albedo, 0, 1);
            float shadow_factor = ComputeShadow(light, world_pos);
            finalColor += light_value * shadow_factor;
        }
    }

    return float4(finalColor, 1.0f);
}
