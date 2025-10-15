struct ClusterBound
{
    float3 minPoint;
    float pad0;
    float3 maxPoint;
    float pad1;
};

struct ClusterLightMeta
{
    uint offset;
    uint count;
};

struct LightInfo
{
    float3 position;
    float range;
    float3 direction;
    float intensity;
    float3 color;
    uint type;
    uint castsShadow;
    uint shadowMapStartIndex;
    uint shadowMapLength;
    uint padding0;
};

cbuffer SceneCB : register(b0)
{
    float4 gTimeInfo;
    
    uint gLightCount;
    uint gRenderFlags;
    float2 padding;
};

cbuffer CameraCB : register(b1)
{
    float4x4 gView;
    float4x4 gProj;
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

#define MAX_LIGHTS_PER_CLUSTER 128

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

    float ndcMinX = (float) x / gClusterCountX * 2.0f - 1.0f;
    float ndcMaxX = (float) (x + 1) / gClusterCountX * 2.0f - 1.0f;
    float ndcMinY = 1.0f - (float) (y + 1) / gClusterCountY * 2.0f;
    float ndcMaxY = 1.0f - (float) y / gClusterCountY * 2.0f;

    float z0 = (float) z / gClusterCountZ;
    float z1 = (float) (z + 1) / gClusterCountZ;
    float viewMinZ = gNearZ * pow(gFarZ / gNearZ, z0);
    float viewMaxZ = gNearZ * pow(gFarZ / gNearZ, z1);

    float4 corners[8] =
    {
        float4(ndcMinX, ndcMinY, 0, 1),
        float4(ndcMaxX, ndcMinY, 0, 1),
        float4(ndcMinX, ndcMaxY, 0, 1),
        float4(ndcMaxX, ndcMaxY, 0, 1),
        float4(ndcMinX, ndcMinY, 1, 1),
        float4(ndcMaxX, ndcMinY, 1, 1),
        float4(ndcMinX, ndcMaxY, 1, 1),
        float4(ndcMaxX, ndcMaxY, 1, 1)
    };

    float3 minP = float3(1e9, 1e9, 1e9);
    float3 maxP = float3(-1e9, -1e9, -1e9);

    for (int i = 0; i < 8; ++i)
    {
        float4 v = corners[i];
        float viewZ = lerp(viewMinZ, viewMaxZ, v.z);
        float4 viewPos = mul(float4(v.x, v.y, viewZ, 1.0f), gInvProj);
        viewPos /= viewPos.w;
        minP = min(minP, viewPos.xyz);
        maxP = max(maxP, viewPos.xyz);
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
[numthreads(8, 8, 1)]
void LightAssignCS(uint3 DTid : SV_DispatchThreadID)
{
    uint x = DTid.x;
    uint y = DTid.y;
    uint z = DTid.z;

    if (x >= gClusterCountX || y >= gClusterCountY || z >= gClusterCountZ)
        return;

    uint clusterIndex = z * (gClusterCountX * gClusterCountY) + y * gClusterCountX + x;

    ClusterBound bound = ClusterListSRV[clusterIndex];
    float3 boxMin = bound.minPoint;
    float3 boxMax = bound.maxPoint;

    uint localCount = 0;
    uint localIndices[MAX_LIGHTS_PER_CLUSTER];

    for (uint i = 0; i < gLightCount; ++i)
    {
        LightInfo light = LightInput[i];

        float3 L = light.position;
        float R = light.range;

        float3 closest = clamp(L, boxMin, boxMax);
        float3 delta = L - closest;
        float dist2 = dot(delta, delta);

        if (dist2 <= R * R)
        {
            if (localCount < MAX_LIGHTS_PER_CLUSTER)
                localIndices[localCount++] = i;
        }
    }

    uint offset = 0;
    InterlockedAdd(GlobalOffsetCounter[0], localCount, offset);

    for (uint k = 0; i < localCount; ++k)
        ClusterLightIndicesUAV[offset + k] = localIndices[k];

    ClusterLightMeta meta;
    meta.offset = offset;
    meta.count = localCount;
    ClusterLightMetaUAV[clusterIndex] = meta;
}
