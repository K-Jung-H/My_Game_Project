cbuffer SceneCB : register(b0)
{
    float deltaTime;
    float totalTime;
    uint frameCount;
    uint padding0;
    uint LightCount;
    uint ClusterIndexCapacity;
    uint RenderFlags;
    uint padding1;
    float4 gAmbientColor;
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

cbuffer ObjectCB : register(b2)
{
    float4x4 gWorld;
    float4 Albedo;
    float Roughness;
    float Metallic;
    float Emissive;
    int DiffuseTexIdx;
    int NormalTexIdx;
    int RoughnessTexIdx;
    int MetallicTexIdx;
};

Texture2D gHeightMap : register(t0);
Texture2D gTextures[2000] : register(t1);

SamplerState gsamClamp : register(s0);
SamplerState gsamWrap : register(s1);

float4 SampleIfValid(int texIdx, float2 uv)
{
    if (texIdx < 0)
        return float4(1, 1, 1, 1);
    return gTextures[texIdx].Sample(gsamWrap, uv);
}

float3 GetRandomColor(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    
    float r = float(seed & 0xFF) / 255.0f;
    float g = float((seed >> 8) & 0xFF) / 255.0f;
    float b = float((seed >> 16) & 0xFF) / 255.0f;
    
    return normalize(float3(r, g, b) + 0.5f);
}

float3 HUEtoRGB(float H)
{
    float R = abs(H * 6.0f - 3.0f) - 1.0f;
    float G = 2.0f - abs(H * 6.0f - 2.0f);
    float B = 2.0f - abs(H * 6.0f - 4.0f);
    return saturate(float3(R, G, B));
}

float3 LODToRainbowColor(float lod)
{
    float normalizedLOD = (lod - 1.0f) / 63.0f;
    float h = lerp(0.66f, 0.0f, normalizedLOD);
    
    return HUEtoRGB(h);
}

struct VS_IN
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float2 UV0 : TEXCOORD0;
    float2 UV1 : TEXCOORD1;
    float4 Color : COLOR;

    float2 InstPos : INST_POS;
    float InstScale : INST_SCALE;
    float InstLOD : INST_LOD;
    
    uint InstanceID : SV_InstanceID;
};

struct VS_OUT
{
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 UV : TEXCOORD0;
    float2 TexScale : TEXCOORD1;
    
    nointerpolation float LOD : INST_LOD;
};

struct HS_TESS_FACTOR
{
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

struct HS_OUT
{
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 UV : TEXCOORD0;
    float2 TexScale : TEXCOORD1;
    
    nointerpolation float LOD : INST_LOD;
};

struct DS_OUT
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 UV : TEXCOORD0;
    
    nointerpolation float3 LODColor : COLOR;
    
    nointerpolation float DebugH : TEXCOORD5;
};

struct PS_OUT
{
    float4 Albedo : SV_Target0;
    float4 Normal : SV_Target1;
    float4 Material : SV_Target2;
};

VS_OUT Default_VS(VS_IN vin)
{
    VS_OUT vout;

    float3 scaledPos = vin.Pos * vin.InstScale;
    float3 worldPos = scaledPos + float3(vin.InstPos.x, 0.0f, vin.InstPos.y);

    vout.PosW = worldPos;
    vout.NormalW = vin.Normal;
    vout.TangentW = vin.Tangent;
    vout.UV = vin.UV0;
    vout.TexScale = float2(vin.InstScale, vin.InstScale);

    vout.LOD = vin.InstLOD;
    
    return vout;
}

HS_TESS_FACTOR CalcHSPatchConstants(InputPatch<VS_OUT, 4> patch)
{
    HS_TESS_FACTOR pt;

    float tessFactor = clamp(patch[0].LOD, 1.0f, 64.0f);
    pt.EdgeTess[0] = tessFactor;
    pt.EdgeTess[1] = tessFactor;
    pt.EdgeTess[2] = tessFactor;
    pt.EdgeTess[3] = tessFactor;
    pt.InsideTess[0] = tessFactor;
    pt.InsideTess[1] = tessFactor;

    return pt;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("CalcHSPatchConstants")]
[maxtessfactor(64.0f)]
HS_OUT Default_HS(InputPatch<VS_OUT, 4> patch, uint i : SV_OutputControlPointID)
{
    HS_OUT hout;
    hout.PosW = patch[i].PosW;
    hout.NormalW = patch[i].NormalW;
    hout.TangentW = patch[i].TangentW;
    hout.UV = patch[i].UV;
    hout.TexScale = patch[i].TexScale;
    hout.LOD = patch[i].LOD;
    
    return hout;
}

[domain("quad")]
DS_OUT Default_DS(HS_TESS_FACTOR patchTess, float2 uv : SV_DomainLocation, const OutputPatch<HS_OUT, 4> quad)
{
    DS_OUT dout;

    float3 v0 = lerp(quad[0].PosW, quad[1].PosW, uv.x);
    float3 v1 = lerp(quad[2].PosW, quad[3].PosW, uv.x);
    float3 p = lerp(v0, v1, uv.y);

    float2 uv0 = lerp(quad[0].UV, quad[1].UV, uv.x);
    float2 uv1 = lerp(quad[2].UV, quad[3].UV, uv.x);
    float2 texUV = lerp(uv0, uv1, uv.y);

    float h = gHeightMap.SampleLevel(gsamClamp, texUV, 0).r;
    
    float heightScale = length(float3(gWorld[0][1], gWorld[1][1], gWorld[2][1]));
    
    if (heightScale == 0.0f)
        heightScale = 1.0f;
    
    p.y += h * heightScale;
    
    p = mul(float4(p, 1.0f), gWorld).xyz;

    dout.PosW = p;
    dout.PosH = mul(float4(p, 1.0f), mul(gView, gProj));
    
    float2 texScale = quad[0].TexScale;
    dout.UV = texUV * texScale;
    
    float3 n0 = lerp(quad[0].NormalW, quad[1].NormalW, uv.x);
    float3 n1 = lerp(quad[2].NormalW, quad[3].NormalW, uv.x);
    float3 normal = normalize(lerp(n0, n1, uv.y));
    dout.NormalW = mul(normal, (float3x3) gWorld);

    float3 t0 = lerp(quad[0].TangentW, quad[1].TangentW, uv.x);
    float3 t1 = lerp(quad[2].TangentW, quad[3].TangentW, uv.x);
    float3 tangent = normalize(lerp(t0, t1, uv.y));
    dout.TangentW = mul(tangent, (float3x3) gWorld);
    
    dout.LODColor = LODToRainbowColor(quad[0].LOD);
    dout.DebugH = h;
    return dout;
}

PS_OUT Default_PS(DS_OUT pin)
{
    //PS_OUT pout;
    
    //float4 albedo = SampleIfValid(DiffuseTexIdx, pin.UV) * Albedo * float4(pin.LODColor, 1.0f);
    //float3 normalT = SampleIfValid(NormalTexIdx, pin.UV).rgb;
    //float roughness = SampleIfValid(RoughnessTexIdx, pin.UV).r * Roughness;
    //float metallic = SampleIfValid(MetallicTexIdx, pin.UV).r * Metallic;

    //normalT = normalT * 2.0f - 1.0f;

    //float3 N = normalize(pin.NormalW);
    //float3 T = normalize(pin.TangentW);
    //float3 B = cross(N, T);
    //float3x3 TBN = float3x3(T, B, N);
    //float3 finalNormal = normalize(mul(normalT, TBN));

    //pout.Albedo = albedo;
    //pout.Normal = float4(finalNormal * 0.5f + 0.5f, 1.0f);
    //pout.Material = float4(roughness, metallic, Emissive, 1.0f);

    //return pout;
    
    PS_OUT pout;
    pout.Albedo = float4(pin.DebugH, pin.DebugH, pin.DebugH, 1.0f);
    return pout;
}

VS_OUT VS_Shadow(VS_IN vin)
{
    VS_OUT vout;

    float3 scaledPos = vin.Pos * vin.InstScale;
    float3 worldPos = scaledPos + float3(vin.InstPos.x, 0.0f, vin.InstPos.y);

    vout.PosW = worldPos;
    vout.NormalW = vin.Normal;
    vout.TangentW = vin.Tangent;
    vout.UV = vin.UV0;
    vout.TexScale = float2(vin.InstScale, vin.InstScale);
    vout.LOD = vin.InstLOD;
    
    return vout;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("CalcHSPatchConstants")]
[maxtessfactor(64.0f)]
HS_OUT HS_Shadow(InputPatch<VS_OUT, 4> patch, uint i : SV_OutputControlPointID)
{
    HS_OUT hout;
    hout.PosW = patch[i].PosW;
    hout.NormalW = patch[i].NormalW;
    hout.TangentW = patch[i].TangentW;
    hout.UV = patch[i].UV;
    hout.TexScale = patch[i].TexScale;
    hout.LOD = patch[i].LOD;
    return hout;
}

[domain("quad")]
DS_OUT DS_Shadow(HS_TESS_FACTOR patchTess, float2 uv : SV_DomainLocation, const OutputPatch<HS_OUT, 4> quad)
{
    DS_OUT dout;

    float3 v0 = lerp(quad[0].PosW, quad[1].PosW, uv.x);
    float3 v1 = lerp(quad[2].PosW, quad[3].PosW, uv.x);
    float3 p = lerp(v0, v1, uv.y);

    float2 uv0 = lerp(quad[0].UV, quad[1].UV, uv.x);
    float2 uv1 = lerp(quad[2].UV, quad[3].UV, uv.x);
    float2 texUV = lerp(uv0, uv1, uv.y);

    float h = gHeightMap.SampleLevel(gsamClamp, texUV, 0).r;
    
    float heightScale = length(float3(gWorld[0][1], gWorld[1][1], gWorld[2][1]));
    if (heightScale == 0.0f)
        heightScale = 1.0f;
    
    p.y += h * heightScale;
    p = mul(float4(p, 1.0f), gWorld).xyz;

    dout.PosW = p;
    dout.PosH = mul(float4(p, 1.0f), mul(gView, gProj));
    dout.UV = texUV;
    dout.NormalW = float3(0, 1, 0);
    dout.TangentW = float3(1, 0, 0);
    dout.LODColor = float3(0, 0, 0);
    return dout;
}