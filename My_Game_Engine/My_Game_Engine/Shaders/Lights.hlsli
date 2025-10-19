//==================================================================
// === 공통 BRDF 계산 함수 ===
//==================================================================
float3 F_Schlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float D_GGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float G_Smith(float NdotV, float NdotL, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float Gv = NdotV / (NdotV * (1.0 - k) + k);
    float Gl = NdotL / (NdotL * (1.0 - k) + k);
    return Gv * Gl;
}

float3 PBR_Lighting(
    float3 L,
    float3 V,
    float3 N,
    float3 radiance,
    float3 Albedo,
    float Metallic,
    float Roughness)
{
    float3 H = normalize(L + V);

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

    float3 F0 = lerp(0.04.xxx, Albedo, Metallic);
    float D = D_GGX(NdotH, Roughness);
    float G = G_Smith(NdotV, NdotL, Roughness);
    float3 F = F_Schlick(VdotH, F0);

    float3 specular = (D * G * F) / (4.0 * NdotV * NdotL + 0.001);
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - Metallic);
    float3 diffuse = kD * Albedo / PI;


    return (diffuse + specular) * radiance * NdotL;
}
