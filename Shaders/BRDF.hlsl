#ifndef __BRDF__
#define __BRDF__

#include "Foundation.hlsl"

// Lambert diffuse BRDF = diffuseAlbedo / PI

// Cook-Torrance specular BRDF = D * G * F / (4.0f * NdotL * NdotV) = D * Vis * F
// Vis = G / (4.0f * NdotL * NdotV)

float D_GGX(float squaredRoughness, float NdotH)
{
	float d = (NdotH * squaredRoughness - NdotH) * NdotH + 1.0f;
	return squaredRoughness / (g_PI * d * d);
}

float V_SmithGGXCorrelated(float squaredRoughness, float NdotV, float NdotL)
{
	float lambdaV = NdotL * sqrt((-NdotV * squaredRoughness + NdotV) * NdotV + squaredRoughness);
	float lambdaL = NdotV * sqrt((-NdotL * squaredRoughness + NdotL) * NdotL + squaredRoughness);
	return 0.5f / (lambdaV + lambdaL);
}

float3 F_Schlick(float3 f0, float LdotH)
{
	return f0 + (1.0f - f0) * pow(1.0f - LdotH, 5.0f);
}

float3 BRDF(float NdotL, float3 N, float3 L, float3 V, float3 baseColor, float metallic, float roughness)
{
	float3 H = normalize(L + V);
	float NdotV = saturate(dot(N, V));
	float NdotH = saturate(dot(N, H));
	float LdotH = saturate(dot(L, H));
	
	float squaredRoughness = roughness * roughness;
	float D = D_GGX(squaredRoughness, NdotH);
	float Vis = V_SmithGGXCorrelated(squaredRoughness, NdotV, NdotL);

	float3 f0 = baseColor * metallic;
	float3 F = F_Schlick(f0, LdotH);

	// Cook-Torrance specular BRDF
	float3 specularBRDF = (D * Vis) * F;

	// Lambert diffuse BRDF
	float3 diffuseAlbedo = (1.0f - metallic) * baseColor;
	float3 diffuseBRDF = ((1.0f - F) * g_1DIVPI) * diffuseAlbedo;

	return (diffuseBRDF + specularBRDF);
}

#endif // __BRDF__