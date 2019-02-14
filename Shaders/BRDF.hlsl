#ifndef __BRDF__
#define __BRDF__

#include "Foundation.hlsl"

// Cook-Torrance specular BRDF = D * G * F / (4.0f * NdotL * NdotV) = D * V * F
// V = G / (4.0f * NdotL * NdotV)

float D_GGX(float squaredRoughness, float NdotH)
{
	float d = (NdotH * squaredRoughness - NdotH) * NdotH + 1.0f;
	return squaredRoughness / (g_PI * d * d);
}

float V_SmithGGXCorrelated(float squaredRoughness, float NdotV, float NdotL)
{
	float lambdaV = NdotL * sqrt(NdotV * (NdotV - NdotV * squaredRoughness) + squaredRoughness);
	float lambdaL = NdotV * sqrt(NdotL * (NdotL - NdotL * squaredRoughness) + squaredRoughness);
	return 0.5f / (lambdaV + lambdaL);
}

float3 F_Schlick(float3 f0, float LdotH)
{
	return f0 + (1.0f - f0) * pow(1.0f - LdotH, 5.0f);
}

#endif // __BRDF__