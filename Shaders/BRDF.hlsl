#ifndef __BRDF__
#define __BRDF__

#include "Foundation.hlsl"

float3 BRDF(float3 normal, float3 dirToViewer, float3 dirToLight,
	float3 baseColor, float metallness, float roughness)
{
	float3 halfVec = normalize(dirToLight + dirToViewer);
	float NdotV = saturate(dot(normal, dirToViewer));
	float NdotH = saturate(dot(normal, halfVec));
	float NdotL = saturate(dot(normal, dirToLight));
	float LdotH = saturate(dot(dirToLight, halfVec));
	float roughnessSquared = roughness * roughness;
	
	// GGX distribution term
	float D = roughnessSquared / (g_PI * pow((NdotH * roughnessSquared - NdotH) * NdotH + 1.0f, 2.0f));
	
	// Smith GGX height correlated visibility term
	float lambdaV = NdotL * sqrt(NdotV * NdotV * (1.0f - roughnessSquared) + roughnessSquared);
	float lambdaL = NdotV * sqrt(NdotL * NdotL * (1.0f - roughnessSquared) + roughnessSquared);
	float V = 0.5f / (lambdaV + lambdaL);
	
	// Fresnel term (Schlick approximation)
	float3 f0 = baseColor * metallness;
	float3 F = f0 + (1.0f - f0) * pow(1.0f - LdotH, 5.0f);

	// Cook-Torrance specular BRDF
	float3 specularBRDF = (D * V) * F;

	// Lambert diffuse BRDF
	float3 diffuseAlbedo = (1.0f - metallness) * baseColor;
	float3 diffuseBRDF = ((1.0f - F) * g_RcpPI) * diffuseAlbedo;
	
	return (diffuseBRDF + specularBRDF);
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
	float3 reflectedLuminance = BSDF * incomingIlluminance;
}

#endif // __BRDF__