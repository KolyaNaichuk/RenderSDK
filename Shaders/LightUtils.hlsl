#ifndef __LIGHT_UTILS__
#define __LIGHT_UTILS__

#include "Foundation.hlsl"

struct SpotLightProps
{
	float4x4 viewProjMatrix;
	float3 radiantIntensity;
	float rcpSquaredRange;
	float3 worldSpacePos;
	float angleFalloffScale;
	float3 worldSpaceDir;
	float angleFalloffOffset;
	float viewNearPlane;
	float rcpViewClipRange;
	float negativeExpShadowMapConstant;
	uint lightID;
};

float3 BRDF(float NdotL, float3 normal, float3 dirToLight, float3 dirToViewer,
	float3 baseColor, float metallic, float roughness)
{
	float3 halfVec = normalize(dirToLight + dirToViewer);
	float NdotV = saturate(dot(normal, dirToViewer));
	float NdotH = saturate(dot(normal, halfVec));
	float LdotH = saturate(dot(dirToLight, halfVec));
	float squaredRoughness = roughness * roughness;

	// GGX distribution term
	float D = squaredRoughness / (g_PI * pow((NdotH * squaredRoughness - NdotH) * NdotH + 1.0f, 2.0f));

	// Smith GGX height correlated visibility term
	float lambdaV = NdotL * sqrt(NdotV * NdotV * (1.0f - squaredRoughness) + squaredRoughness);
	float lambdaL = NdotV * sqrt(NdotL * NdotL * (1.0f - squaredRoughness) + squaredRoughness);
	float V = 0.5f / (lambdaV + lambdaL);

	// Fresnel term (Schlick approximation)
	float3 f0 = baseColor * metallic;
	float3 F = f0 + (1.0f - f0) * pow(1.0f - LdotH, 5.0f);

	// Cook-Torrance specular BRDF
	float3 specularBRDF = (D * V) * F;

	// Lambert diffuse BRDF
	float3 diffuseAlbedo = (1.0f - metallic) * baseColor;
	float3 diffuseBRDF = ((1.0f - F) * g_1DIVPI) * diffuseAlbedo;

	return (diffuseBRDF + specularBRDF);
}

float CalcDistanceFalloff(float squaredDistToLight, float lightRcpSquaredRange)
{
	float rcpSquareDistFalloff = 1.0f / max(squaredDistToLight, 0.01f * 0.01f);

	float factor = squaredDistToLight * lightRcpSquaredRange;
	float smoothFactor = saturate(1.0f - factor * factor);

	return rcpSquareDistFalloff * (smoothFactor * smoothFactor);
}

float CalcAngleFalloff(float3 dirToPosition, float3 lightDir, float angleFalloffScale, float angleFalloffOffset)
{
	float cosAngle = dot(dirToPosition, lightDir);
	float factor = saturate(cosAngle * angleFalloffScale + angleFalloffOffset);

	return factor * factor;
}

float3 CalcPointLightContribution(
	float visibility, float3 lightPos, float3 radiantIntensity, float lightRcpSquaredRange,
	float3 position, float3 normal, float3 dirToViewer,
	float3 baseColor, float metallic, float roughness)
{
	float3 reflectedRadiance = 0.0f;
	
	float3 dirToLight = lightPos - position;
	float squaredDistToLight = dot(dirToLight, dirToLight);
	dirToLight /= sqrt(squaredDistToLight);

	float NdotL = dot(normal, dirToLight);
	if (NdotL > 0.0f)
	{
		float distFalloff = CalcDistanceFalloff(squaredDistToLight, lightRcpSquaredRange);

		float3 incidentRadiance = distFalloff * radiantIntensity;
		float3 brdf = BRDF(NdotL, normal, dirToLight, dirToViewer, baseColor, metallic, roughness);

		reflectedRadiance = incidentRadiance * brdf * (visibility * NdotL);
	}

	return reflectedRadiance;
}

float3 CalcSpotLightContribution(
	float visibility, float3 lightPos, float3 lightDir, float3 radiantIntensity,
	float lightRcpSquaredRange, float angleFalloffScale, float angleFalloffOffset,
	float3 position, float3 normal, float3 dirToViewer,
	float3 baseColor, float metallic, float roughness)
{
	float3 reflectedRadiance = 0.0f;

	float3 dirToLight = lightPos - position;
	float squaredDistToLight = dot(dirToLight, dirToLight);
	dirToLight /= sqrt(squaredDistToLight);

	float NdotL = dot(normal, dirToLight);
	if (NdotL > 0.0f)
	{
		float distFalloff = CalcDistanceFalloff(squaredDistToLight, lightRcpSquaredRange);
		float angleFalloff = CalcAngleFalloff(-dirToLight, lightDir, angleFalloffScale, angleFalloffOffset);

		float3 incidentRadiance = (distFalloff * angleFalloff) * radiantIntensity;
		float3 brdf = BRDF(NdotL, normal, dirToLight, dirToViewer, baseColor, metallic, roughness);

		reflectedRadiance = incidentRadiance * brdf * (visibility * NdotL);
	}

	return reflectedRadiance;
}

float3 CalcDirectionalLightContribution(
	float3 dirToLight, float3 irradiancePerpToLightDir, float3 normal, float3 dirToViewer,
	float3 baseColor, float metallic, float roughness)
{
	float3 reflectedRadiance = 0.0f;

	float NdotL = dot(normal, dirToLight);
	if (NdotL > 0.0f)
	{
		float3 irradianceAtSurface = irradiancePerpToLightDir * NdotL;
		float3 brdf = BRDF(NdotL, normal, dirToLight, dirToViewer, baseColor, metallic, roughness);

		reflectedRadiance = irradianceAtSurface * brdf;
	}

	return reflectedRadiance;
}

float3 CalcPhongLighting(float3 dirToViewer, float3 dirToLight, float3 lightColor,
	float3 normal, float3 diffuseAlbedo, float3 specularAlbedo, float shininess)
{
	float NdotL = saturate(dot(normal, dirToLight));
	float3 diffuseColor = NdotL * diffuseAlbedo * lightColor;

	float3 reflectDir = normalize(reflect(-dirToLight, normal));
	float RdotV = saturate(dot(reflectDir, dirToViewer));
	float3 specularColor = (NdotL * pow(RdotV, shininess)) * specularAlbedo * lightColor;

	return (diffuseColor + specularColor);
}

float3 CalcBlinnPhongLighting(float3 dirToViewer, float3 dirToLight, float3 lightColor,
	float3 normal, float3 diffuseAlbedo, float3 specularAlbedo, float shininess)
{
	float NdotL = saturate(dot(normal, dirToLight));
	float3 diffuseColor = NdotL * diffuseAlbedo * lightColor;

	float3 halfVec = normalize(dirToLight + dirToViewer);
	float HdotN = saturate(dot(halfVec, normal));
	float3 specularColor = (NdotL * pow(HdotN, shininess)) * specularAlbedo * lightColor;

	return (diffuseColor + specularColor);
}

#endif // __LIGHT_UTILS__