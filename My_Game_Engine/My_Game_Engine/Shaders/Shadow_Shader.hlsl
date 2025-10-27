
uint Index : register(b0);

cbuffer ObjectCB : register(b1)
{
    float4x4 World;
    float4 Albedo;
    float Roughness;
    float Metallic;
    float Emissive;
    float padding0;
    int DiffuseTexIdx;
    int NormalTexIdx;
    int RoughnessTexIdx;
    int MetallicTexIdx;
};

struct ShadowMatrixData
{
    float4x4 ViewProj;
};

StructuredBuffer<ShadowMatrixData> ShadowMatrixBuffer : register(t0);


struct VS_IN
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float3 TangentL : TANGENT; 
    float2 TexC : TEXCOORD; 
    float4 Color : COLOR; 
};


struct VS_OUT
{
    float4 PosH : SV_Position;
};

VS_OUT Shadow_VS(VS_IN vin)
{
    VS_OUT vout;

    float4x4 lightViewProj = ShadowMatrixBuffer[Index].ViewProj;
    float4 posW = mul(float4(vin.PosL, 1.0f), World);
    vout.PosH = mul(posW, lightViewProj);
    return vout;
    
    
}
