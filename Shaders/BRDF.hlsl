#ifndef __BRDF__
#define __BRDF__

#include "Foundation.hlsl"

// illuminance = luminousIntensity / max(squaredDist, 0.01f * 0.01f);

// point light
// luminousPower
// luminousIntensity = luminousPower / (4.0f * PI);

// spot light
// openingAngle
// luminousPower
// luminousIntensity = luminousPower / (2.0f * PI * (1.0f - cos(0.5f * openingAngle)))

float3 BRDF(float3 normal, float3 dirToViewer, float3 dirToLight,
	float3 baseColor, float metallness, float roughness)
{
	if (NdotL > 0.0f)
	{
		float3 halfVec = normalize(dirToLight + dirToViewer);
		float NdotV = saturate(dot(normal, dirToViewer));
		float NdotH = saturate(dot(normal, halfVec));
		float NdotL = saturate(dot(normal, dirToLight));
		float LdotH = saturate(dot(dirToLight, halfVec));
		float squaredRoughness = roughness * roughness;

		// GGX distribution term
		float D = squaredRoughness / (g_PI * pow((NdotH * squaredRoughness - NdotH) * NdotH + 1.0f, 2.0f));

		// Smith GGX height correlated visibility term
		float lambdaV = NdotL * sqrt(NdotV * NdotV * (1.0f - squaredRoughness) + squaredRoughness);
		float lambdaL = NdotV * sqrt(NdotL * NdotL * (1.0f - squaredRoughness) + squaredRoughness);
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
	return 0.0f;
}

float CalcDistanceFalloff(float squaredDistToLight, float lightRcpSquaredRange)
{
	float rcpSquareDistFalloff = 1.0f / max(lightRcpSquaredRange, 0.01f * 0.01f);
	
	float factor = squaredDistToLight * rcpSquaredLightRange;
	float smoothFactor = saturate(1.0f - factor * factor);

	return rcpSquareDistFalloff * (smoothFactor * smoothFactor);
}

float CalcAngleFalloff(float3 dirToPosition, float3 lightDir, float angleFalloffScale, float angleFalloffOffset)
{
	// innerConeAngle <= outerConeAngle
	// outerConeAngle <= 180 degrees

	// angleFalloffScale = 1.0f / max(0.001f, cosInnerConeAngle - cosOuterConeAngle);
	// angleFalloffOffset = -cosOuterConeAngle * angleFalloffScale; 

	float cosAngle = dot(dirToPosition, lightDir);
	float factor = saturate(cosAngle * angleFalloffScale + angleFalloffOffset);
	
	return factor * factor;
}

float3 CalcPointLightContribution(
	float3 lightPos, float3 radianceIntensity, float lightRcpSquaredRange,
	float3 position, float3 normal, float3 dirToViewer,
	float3 baseColor, float metallness, float roughness)
{
	float3 dirToLight = lightPos - position;
	float distToLight = length(dirToLight);
	dirToLight /= distToLight;

	float distFalloff = CalcDistanceFalloff(distToLight * distToLight, lightRcpSquaredRange);
	
	float3 incidentRadiance = distFalloff * radianceIntensity;
	float3 brdf = BRDF(normal, dirToViewer, dirToLight, baseColor, metallness, roughness);
	float NdotL = saturate(dot(normal, dirToLight));
	
	float3 reflectedRadiance = incidentRadiance * brdf * NdotL;
	return reflectedRadiance;
}

float3 CalcSpotLightContribution(
	float3 lightPos, float3 lightDir, float3 radianceIntensity,
	float lightRcpSquaredRange, float angleFalloffScale, float angleFalloffOffset,
	float3 position, float3 normal, float3 dirToViewer,
	float3 baseColor, float metallness, float roughness)
{
	float3 dirToLight = lightPos - position;
	float distToLight = length(dirToLight);
	dirToLight /= distToLight;

	float distFalloff = CalcDistanceFalloff(distToLight * distToLight, lightRcpSquaredRange);
	float angleFalloff = CalcAngleFalloff(-dirToLight, lightDir, angleFalloffScale, angleFalloffOffset);

	float3 incidentRadiance = (distFalloff * angleFalloff) * radianceIntensity;
	float3 brdf = BRDF(normal, dirToViewer, dirToLight, baseColor, metallness, roughness);
	float NdotL = saturate(dot(normal, dirToLight));

	float3 reflectedRadiance = incidentRadiance * brdf * NdotL;
	return reflectedRadiance;
}

// Moving frostbite to PBR shading
// In Frostbite engine, all lights use photometric units, implying that our rendering pipeline stores luminance values in render targets - page 26.

// Page 28. Table 8. Light units associated with various Frostbite types.
// Punctual - luminous power
// Sun - illuminance (in lux) for a surface perpendicular to the sun direction.

// Page 36. See the table for sun/sky illuminance values.
float3 CalcSunLightContribution()
{
	float sunIlluminance = ;
	float NdotL = saturate(dot(worldSpaceNormal, sunWorldSpaceDir));

	float incomingIlluminance = sunIlluminance * NdotL;
	float3 reflectedLuminance = BSDF * incomingIlluminance;
}

#endif // __BRDF__