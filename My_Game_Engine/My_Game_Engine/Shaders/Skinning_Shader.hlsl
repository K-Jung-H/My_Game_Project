
struct BoneMatrixData
{
    float4x4 transform;
};

struct SkinData
{
    uint4 idx; 
    uint4 w16; 
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
ByteAddressBuffer g_SkinData : register(t0);
ByteAddressBuffer g_InputVertices : register(t1);

RWByteAddressBuffer g_OutputVertices : register(u0);


float4 ConvertWeights(uint w0, uint w1)
{
    float4 weights;
    weights.x = (w0 & 0xFFFF) / 65535.0f;
    weights.y = (w0 >> 16) / 65535.0f;
    weights.z = (w1 & 0xFFFF) / 65535.0f;
    weights.w = (w1 >> 16) / 65535.0f;
    return weights;
}

uint4 LoadIndices(uint offset)
{
    uint2 data = g_SkinData.Load2(offset);
    return uint4(
        data.x & 0xFFFF,
        (data.x >> 16) & 0xFFFF,
        data.y & 0xFFFF,
        (data.y >> 16) & 0xFFFF
    );
}

float4 LoadWeights(uint offset)
{
    uint2 data = g_SkinData.Load2(offset + 8);
    return ConvertWeights(data.x, data.y);
}


[numthreads(64, 1, 1)]
void Skinning_CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint vertexID = dispatchThreadID.x;
    
    // g_Constants.vertexCount는 버퍼 오버플로우 방지용 (Renderer.cpp에서 전달 필요)
    if (vertexID >= g_Constants.vertexCount)
    {
        return;
    }

    // --- 1. 입력 데이터 읽기 ---
    
    // 1.1. 스킨 가중치/인덱스 읽기 (t0)
    // C++의 GPU_SkinData stride = 16 바이트 (idx[8] + w16[8])
    uint skinDataOffset = vertexID * 16;
    uint4 boneIndices = LoadIndices(skinDataOffset);
    float4 boneWeights = LoadWeights(skinDataOffset);

    // 1.2. 원본 정점 읽기 (t1)
    // C++의 mHotLayout.stride = 36 바이트
    uint hotVbOffset = vertexID * 36; // g_Constants.hotStride 사용 권장
    float3 pos = g_InputVertices.Load3(hotVbOffset + 0); // Position (offset 0)
    float3 nml = g_InputVertices.Load3(hotVbOffset + 12); // Normal (offset 12)
    float3 tan = g_InputVertices.Load3(hotVbOffset + 24); // Tangent (offset 24)


    // --- 2. 스키닝 계산 ---
    
    float3 skinnedPos = (float3) 0.0f;
    float3 skinnedNml = (float3) 0.0f;
    float3 skinnedTan = (float3) 0.0f;

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        if (boneWeights[i] > 0.0f)
        {
            // 2.1. 본 행렬 읽기 (t2)
            // C++: final = XMMatrixTranspose(invBind * global)
            // HLSL: C++에서 이미 Transpose했으므로, 그대로 사용
            float4x4 boneTransform = g_BoneMatrices[boneIndices[i]].transform;

            // 2.2. 위치 변환 (벡터가 아닌 '점'으로 변환, w=1)
            skinnedPos += mul(float4(pos, 1.0f), boneTransform).xyz * boneWeights[i];

            // 2.3. 노멀/탄젠트 변환 (벡터로 변환, w=0)
            skinnedNml += mul(float4(nml, 0.0f), boneTransform).xyz * boneWeights[i];
            skinnedTan += mul(float4(tan, 0.0f), boneTransform).xyz * boneWeights[i];
        }
    }

    // --- 3. 출력 (UAV) ---
    
    // (노멀/탄젠트 정규화는 선택 사항. 보통은 정규화함)
    skinnedNml = normalize(skinnedNml);
    skinnedTan = normalize(skinnedTan);

    // 1.2에서 읽은 offset과 동일한 위치에 덮어쓰기
    g_OutputVertices.Store3(hotVbOffset + 0, skinnedPos);
    g_OutputVertices.Store3(hotVbOffset + 12, skinnedNml);
    g_OutputVertices.Store3(hotVbOffset + 24, skinnedTan);
}