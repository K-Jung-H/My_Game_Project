struct BoneMatrixData
{
    float4x4 transform;
};

struct SkinData
{
    uint4 idx_w16;
};

struct Vertex_Hot
{
    float3 Position;
    float3 Normal;
    float3 Tangent;
};

struct SkinningConstants
{
    unsigned int vertexCount;
    unsigned int hotStride;
    unsigned int skinStride;
    unsigned int boneCount;
};

ConstantBuffer<SkinningConstants> g_Constants : register(b0);
StructuredBuffer<BoneMatrixData> g_BoneMatrices : register(t2);
StructuredBuffer<SkinData> g_SkinData : register(t0);
StructuredBuffer<Vertex_Hot> g_InputVertices : register(t1);
RWStructuredBuffer<Vertex_Hot> g_OutputVertices : register(u0);

float4 ConvertWeights(uint w0, uint w1)
{
    float4 weights;
    weights.x = (w0 & 0xFFFF) / 65535.0f;
    weights.y = (w0 >> 16) / 65535.0f;
    weights.z = (w1 & 0xFFFF) / 65535.0f;
    weights.w = (w1 >> 16) / 65535.0f;
    return weights;
}

[numthreads(64, 1, 1)]
void Skinning_CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint vertexID = dispatchThreadID.x;
    
    if (vertexID >= g_Constants.vertexCount)
    {
        return;
    }

    SkinData skin = g_SkinData[vertexID];
    uint4 boneIndices = uint4(
        skin.idx_w16.x & 0xFFFF,
        (skin.idx_w16.x >> 16) & 0xFFFF,
        skin.idx_w16.y & 0xFFFF,
        (skin.idx_w16.y >> 16) & 0xFFFF
    );
    float4 boneWeights = ConvertWeights(skin.idx_w16.z, skin.idx_w16.w);

    Vertex_Hot inputVert = g_InputVertices[vertexID];
    float3 pos = inputVert.Position;
    float3 nml = inputVert.Normal;
    float3 tan = inputVert.Tangent;
    
    float totalWeight = boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w;

    float3 skinnedPos = (float3) 0.0f;
    float3 skinnedNml = (float3) 0.0f;
    float3 skinnedTan = (float3) 0.0f;

    if (totalWeight > 0.0f)
    {
        [unroll]
        for (int i = 0; i < 4; ++i)
        {
            if (boneWeights[i] > 0.0f)
            {
                float4x4 boneTransform = g_BoneMatrices[boneIndices[i]].transform;
                skinnedPos += mul(float4(pos, 1.0f), boneTransform).xyz * boneWeights[i];
                skinnedNml += mul(float4(nml, 0.0f), boneTransform).xyz * boneWeights[i];
                skinnedTan += mul(float4(tan, 0.0f), boneTransform).xyz * boneWeights[i];
            }
        }

        skinnedNml = (length(skinnedNml) > 0.0001f) ? normalize(skinnedNml) : float3(0, 1, 0);
        skinnedTan = (length(skinnedTan) > 0.0001f) ? normalize(skinnedTan) : float3(1, 0, 0);
    }
    else
    {
        skinnedPos = pos;
        skinnedNml = nml;
        skinnedTan = tan;
    }
    
    Vertex_Hot outputVert;
    outputVert.Position = skinnedPos;
    outputVert.Normal = skinnedNml;
    outputVert.Tangent = skinnedTan;
    g_OutputVertices[vertexID] = outputVert;
}