
struct ClusterBound
{
    float3 minPoint;
    float pad0;
    float3 maxPoint;
    float pad1;
};

struct LightInfo
{
    float3 position;
    float range;

    float3 direction;
    float intensity;

    float3 color;
    uint type;

    float spotOuterCosAngle;
    float spotInnerCosAngle;
    uint castsShadow;
    uint lightMask;
    
    float volumetricStrength;
    uint shadowMapStartIndex;
    uint shadowMapLength;
    uint padding;
};

struct ClusterLightMeta
{
    uint offset;
    uint count;
    float2 padding0;
};

cbuffer SceneCB : register(b0)
{
    float4 gTimeInfo;
    
    uint gLightCount;
    uint gClusterIndexCapacity;
    uint gRenderFlags;
    float padding;
};

cbuffer CameraCB : register(b1)
{
    float4x4 gView;
    float4x4 gProj;
    float4x4 gInvView;
    float4x4 gInvProj;
    float4x4 gInvViewProj;
    float3 gCameraPos;
    float gPadding0;

    float gNearZ;
    float gFarZ;
    float2 gPadding1;

    uint gClusterCountX;
    uint gClusterCountY;
    uint gClusterCountZ;
    float gPadding2;
};

StructuredBuffer<LightInfo> LightInput : register(t0);
StructuredBuffer<ClusterBound> ClusterListSRV : register(t1);

RWStructuredBuffer<ClusterBound> ClusterListUAV : register(u0);
RWStructuredBuffer<ClusterLightMeta> ClusterLightMetaUAV : register(u1);
RWStructuredBuffer<uint> ClusterLightIndicesUAV : register(u2);

RWStructuredBuffer<uint> GlobalOffsetCounter : register(u3);


uint TotalClusters()
{
    return gClusterCountX * gClusterCountY * gClusterCountZ;
}

uint3 Cluster3DFromLinear(uint idx)
{
    uint xy = gClusterCountX * gClusterCountY;
    uint z = idx / xy;
    uint r = idx - z * xy;
    uint y = r / gClusterCountX;
    uint x = r - y * gClusterCountX;
    return uint3(x, y, z);
}

// -----------------------------------------------------------------------------
// LightClusterClearCS : 각 Cluster 에 저장된 내용 초기화
// -----------------------------------------------------------------------------
[numthreads(64, 1, 1)]
void LightClusterClearCS(uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
    uint idx = Gid.x * 64u + GTid.x;

    // Clear global offset counter exactly once
    if (Gid.x == 0 && GTid.x == 0)
    {
        GlobalOffsetCounter[0] = 0;
    }

    // Clear per-cluster meta (offset/count = 0)
    uint total = TotalClusters();
    if (idx < total)
    {
        ClusterLightMeta m;
        m.offset = 0;
        m.count = 0;
        m.padding0 = float2(0.0f, 0.0f);
        ClusterLightMetaUAV[idx] = m;
    }
}


// -----------------------------------------------------------------------------
// ClusterBoundsBuildCS : 클러스터의 View-space AABB 계산
// -----------------------------------------------------------------------------
[numthreads(8, 8, 1)]
void ClusterBoundsBuildCS(uint3 DTid : SV_DispatchThreadID)
{
    uint x = DTid.x;
    uint y = DTid.y;
    uint z = DTid.z;

    if (x >= gClusterCountX || y >= gClusterCountY || z >= gClusterCountZ)
        return;

    uint index = z * (gClusterCountX * gClusterCountY) + y * gClusterCountX + x;

    // NDC tile
    float ndcMinX = ((float) x / gClusterCountX) * 2.0f - 1.0f;
    float ndcMaxX = ((float) (x + 1u) / gClusterCountX) * 2.0f - 1.0f;
    float ndcMaxY = 1.0f - ((float) y / gClusterCountY) * 2.0f;
    float ndcMinY = 1.0f - ((float) (y + 1u) / gClusterCountY) * 2.0f;

    // Exponential partition along Z in view space
    float z0 = (float) z / gClusterCountZ;
    float z1 = (float) (z + 1) / gClusterCountZ;
    float viewMinZ = gNearZ * pow(gFarZ / gNearZ, z0);
    float viewMaxZ = gNearZ * pow(gFarZ / gNearZ, z1);

    // Compute 4 view rays from NDC corners at clip.w = 1, then scale by depths
    float4 clip00 = float4(ndcMinX, ndcMinY, 1.0f, 1.0f);
    float4 clip10 = float4(ndcMaxX, ndcMinY, 1.0f, 1.0f);
    float4 clip01 = float4(ndcMinX, ndcMaxY, 1.0f, 1.0f);
    float4 clip11 = float4(ndcMaxX, ndcMaxY, 1.0f, 1.0f);

    float4 v00 = mul(clip00, gInvProj);
    v00.xyz /= max(v00.w, 1e-6);
    float4 v10 = mul(clip10, gInvProj);
    v10.xyz /= max(v10.w, 1e-6);
    float4 v01 = mul(clip01, gInvProj);
    v01.xyz /= max(v01.w, 1e-6);
    float4 v11 = mul(clip11, gInvProj);
    v11.xyz /= max(v11.w, 1e-6);

    // Normalize direction along view rays (z>0 in view space)
    float3 ray00 = normalize(v00.xyz);
    float3 ray10 = normalize(v10.xyz);
    float3 ray01 = normalize(v01.xyz);
    float3 ray11 = normalize(v11.xyz);

    // 8 corners in view space
    float3 corners[8];
    corners[0] = ray00 * viewMinZ;
    corners[1] = ray10 * viewMinZ;
    corners[2] = ray01 * viewMinZ;
    corners[3] = ray11 * viewMinZ;
    corners[4] = ray00 * viewMaxZ;
    corners[5] = ray10 * viewMaxZ;
    corners[6] = ray01 * viewMaxZ;
    corners[7] = ray11 * viewMaxZ;

    float3 minP = float3(1e9, 1e9, 1e9);
    float3 maxP = float3(-1e9, -1e9, -1e9);
    [unroll]
    for (int i = 0; i < 8; i++)
    {
        minP = min(minP, corners[i]);
        maxP = max(maxP, corners[i]);
    }

    ClusterBound cb;
    cb.minPoint = minP;
    cb.maxPoint = maxP;
    cb.pad0 = 0;
    cb.pad1 = 0;

    
    
    ClusterListUAV[index] = cb;
}

// -----------------------------------------------------------------------------
// LightAssignCS : 각 Cluster에 포함된 Light 인덱스를 계산
// -----------------------------------------------------------------------------
[numthreads(64, 1, 1)]
void LightAssignCS(uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
    uint MAX_LIGHTS_PER_CLUSTER = gClusterIndexCapacity;
    
    uint clusterIndex = Gid.x * 64u + GTid.x;
    uint total = TotalClusters();
    if (clusterIndex >= total)
        return;

    ClusterBound bound = ClusterListSRV[clusterIndex];
    float3 boxMin = bound.minPoint;
    float3 boxMax = bound.maxPoint;

    uint localCount = 0;
    uint localIndices[64];


    [loop]
    for (uint li = 0; li < gLightCount; ++li)
    {
        LightInfo light = LightInput[li];
        bool intersects = false;

        switch (light.type)
        {
            case 0: // Directional Light
            {
                    intersects = true;
                    break;
                }

            // ────────────────────────────────────────────────
            case 1: // Point Light (Sphere vs AABB)
            {
                    float3 closestPoint = clamp(light.position, boxMin, boxMax);
                    float3 delta = light.position - closestPoint;
                    float distSq = dot(delta, delta);
                    intersects = (distSq <= light.range * light.range);
                    break;
                }

            // ────────────────────────────────────────────────
            case 2: // Spot Light (Cone vs AABB)
            {
                    float coneRadius = light.range * tan(acos(light.spotOuterCosAngle));
                    float3 coneEnd = light.position + light.direction * light.range;

                    float3 sphereCenter = (light.position + coneEnd) * 0.5f;
                    float sphereRadius = length(coneEnd - light.position) * 0.5f + coneRadius * 0.5f;

                    float3 closestPoint = clamp(sphereCenter, boxMin, boxMax);
                    float3 delta = sphereCenter - closestPoint;
                    float distSq = dot(delta, delta);
                    intersects = (distSq <= sphereRadius * sphereRadius);
                    break;
                }

            // ────────────────────────────────────────────────
            default:
                break;
        }

        if (intersects)
        {
            if (localCount < MAX_LIGHTS_PER_CLUSTER)
                localIndices[localCount++] = li;

            if (localCount == MAX_LIGHTS_PER_CLUSTER)
                break;
        }
    }


    uint offset = 0;
    InterlockedAdd(GlobalOffsetCounter[0], localCount, offset);

    uint totalCapacity = TotalClusters() * gClusterIndexCapacity;
    uint writeCount = 0;
    if (offset < totalCapacity)
        writeCount = min(localCount, totalCapacity - offset);

    [loop]
    for (uint k = 0; k < writeCount; ++k)
        ClusterLightIndicesUAV[offset + k] = localIndices[k];

    ClusterLightMeta meta;
    meta.offset = offset;
    meta.count = writeCount;
    meta.padding0 = float2(0.0f, 0.0f);
    ClusterLightMetaUAV[clusterIndex] = meta;
}
