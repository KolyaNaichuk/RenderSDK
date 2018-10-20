#ifndef __BRDF__
#define __BRDF__

#include "Foundation.hlsl"

// f0 - specular reflectance at normal incidence.
// Refer to this as the specular color of the surface.
// The values are between 0 and 1.
float3 F_Schlick(float LdotH, float3 f0)
{
	return f0 + (1.0f - f0) * pow(1.0f - LdotH, 5.0f);
}

float D_GGX(float NdotH, float roughness)
{
	float roughnessSquared = roughness * roughness;
	float f = (NdotH * roughnessSquared - NdotH) * NdotH + 1.0f;
	
	return roughnessSquared / (g_PI * f * f);
}

float V_SmithGGXHeightCorrelated(float NdotV, float NdotL, float roughness)
{
	float roughnessSquared = roughness * roughness;
	float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0f - roughnessSquared) + roughnessSquared);
	float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0f - roughnessSquared) + roughnessSquared);
	
	return 0.5f / (GGXV + GGXL);
}

float3 Fd_Lambert()
{
	return g_RcpPI;
}

// Moving frostbite to PBR shading
// In Frostbite engine, all lights use photometric units, implying that our rendering pipeline stores luminance values in render targets - page 26.

// Page 28. Table 8. Light units associated with various Frostbite types.
// Punctual - luminous power
// Sun - illuminance (in lux) for a surface perpendicular to the sun direction.

// Page 36. See the table for sun/sky illuminance values.
float3 CalcSunLightingContribution()
{
	float sunIlluminance = ;
	float NdotL = saturate(dot(worldSpaceNormal, sunWorldSpaceDir));

	float incomingIlluminance = sunIlluminance * NdotL;
	float3 outgoingLuminance = BSDF * incomingIlluminance;
}

#endif // __BRDF__