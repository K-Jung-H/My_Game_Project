cbuffer SceneCB : register(b0)
{
    float4 gTimeInfo;
};

cbuffer ObjectCB : register(b1)
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

cbuffer CameraCB : register(b2)
{
    float4x4 gView;
    float4x4 gProj;
    float3 gCameraPos;
    float _pad0;
};


Texture2D gTextures[2000] : register(t0); 
SamplerState gLinearSampler : register(s0);

struct VS_IN
{
    float3 pos : POSITION;
    float3 n : NORMAL;
    float3 t : TANGENT;
    float2 uv : TEXCOORD0;
};
struct VS_OUT
{
    float4 svPos : SV_POSITION;
    float3 nWS : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

VS_OUT Default_VS(VS_IN i)
{
    VS_OUT o;
    float4 wpos = mul(float4(i.pos, 1), gWorld);
    float4 vpos = mul(wpos, gView);
    o.svPos = mul(vpos, gProj);
    o.nWS = mul(float4(i.n, 0), gWorld).xyz;
    o.uv = i.uv;
    return o;
}

struct PS_OUT
{
    float4 Albedo : SV_Target0;
    float4 NormalRG : SV_Target1;
    float4 MetalEmis : SV_Target2;
};

float4 SampleIfValid(int idx, float2 uv)
{
    if (idx < 0)
    {
        return float4(1, 1, 1, 1);
    }
    
    uint uidx = (uint) idx;
    return gTextures[NonUniformResourceIndex(uidx)].Sample(gLinearSampler, uv);
}

PS_OUT Default_PS(VS_OUT i)
{
    PS_OUT o;

    float4 texAlbedo = SampleIfValid(DiffuseTexIdx, i.uv);
    o.Albedo = texAlbedo * Albedo; 

    float3 n = normalize(i.nWS);
    o.NormalRG.xyz = 0.5f * (n + 1.0f);


    float rough = Roughness;
    if (RoughnessTexIdx >= 0)
        rough *= SampleIfValid(RoughnessTexIdx, i.uv).r;
    o.NormalRG.w = saturate(rough);


    float metal = Metallic;
    if (MetallicTexIdx >= 0)
        metal *= SampleIfValid(MetallicTexIdx, i.uv).r;
    o.MetalEmis.x = saturate(metal);
    o.MetalEmis.y = Emissive; 
    o.MetalEmis.zw = 0;

    return o;
}


//struct VS_SCREEN_OUT
//{
//    float4 position : SV_POSITION;
//    float2 uv : TEXCOORD0;
//};

//static const VS_SCREEN_OUT g_FullscreenQuadCorners[4] =
//{
//    { float4(-1.0, 1.0, 0.0, 1.0), float2(0.0, 0.0) }, // top-left
//    { float4(1.0, 1.0, 0.0, 1.0), float2(1.0, 0.0) }, // top-right
//    { float4(1.0, -1.0, 0.0, 1.0), float2(1.0, 1.0) }, // bottom-right
//    { float4(-1.0, -1.0, 0.0, 1.0), float2(0.0, 1.0) } // bottom-left
//};

//VS_SCREEN_OUT FullscreenQuad_VS(uint vid : SV_VertexID)
//{
//    static const uint indices[6] = { 0, 1, 2, 0, 2, 3 };

//    return g_FullscreenQuadCorners[indices[vid]];
//}


//VS_SCREEN_OUT Default_VS(uint nVertexID : SV_VertexID)
//{
//    VS_SCREEN_OUT output;
//    output = FullscreenQuad_VS(nVertexID);
  
//    return output;
//}

//float4 Default_PS(VS_SCREEN_OUT input) : SV_TARGET
//{
//    float3 colorTexture = gTextures[DiffuseTexIdx].Sample(gLinearSampler, input.uv);
//    return float4(colorTexture, 1.0f);
//}

