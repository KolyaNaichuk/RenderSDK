#ifndef __LIGHTING__
#define __LIGHTING__

#define SHADING_MODE_PHONG			1
#define SHADING_MODE_BLINN_PHONG	2

struct PointLightProps
{
	float3 color;
	float attenStartRange;
};

struct SpotLightProps
{
	float3 color;
	float3 worldSpaceDir;
	float attenStartRange;
	float attenEndRange;
	float cosHalfInnerConeAngle;
	float cosHalfOuterConeAngle;
};

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

	float3 halfDir = normalize(dirToLight + dirToViewer);
	float HdotN = saturate(dot(halfDir, normal));
	float3 specularColor = (NdotL * pow(HdotN, shininess)) * specularAlbedo * lightColor;

	return (diffuseColor + specularColor);
}

float3 CalcLighting(float3 dirToViewer, float3 dirToLight, float3 lightColor,
	float3 normal, float3 diffuseAlbedo, float3 specularAlbedo, float shininess)
{
#if SHADING_MODE == SHADING_MODE_PHONG
	return CalcPhongLighting(dirToViewer, dirToLight, lightColor, normal, diffuseAlbedo, specularAlbedo, shininess);
#endif

#if SHADING_MODE == SHADING_MODE_BLINN_PHONG
	return CalcBlinnPhongLighting(dirToViewer, dirToLight, lightColor, normal, diffuseAlbedo, specularAlbedo, shininess);
#endif
}

float CalcLightDistanceAttenuation(float distToLight, float attenStartRange, float attenEndRange)
{
	return saturate((attenEndRange - distToLight) / (attenEndRange - attenStartRange));
}

float CalcLightDirectionAttenuation(float cosDirAngle, float cosHalfInnerConeAngle, float cosHalfOuterConeAngle)
{
	return smoothstep(cosHalfOuterConeAngle, cosHalfInnerConeAngle, cosDirAngle);
}

float3 CalcPointLightContribution(float3 lightPos, float3 lightColor, float attenStartRange, float attenEndRange,
	float3 dirToViewer, float3 position, float3 normal, float3 diffuseAlbedo, float3 specularAlbedo, float shininess)
{
	float3 dirToLight = lightPos - position;
	float distToLight = length(dirToLight);
	dirToLight *= rcp(distToLight);

	float3 lightContrib = CalcLighting(dirToViewer, dirToLight, lightColor, normal, diffuseAlbedo, specularAlbedo, shininess);
	float distAtten = CalcLightDistanceAttenuation(distToLight, attenStartRange, attenEndRange);

	return distAtten * lightContrib;
}

float3 CalcSpotLightContribution(float3 lightPos, float3 lightDir, float3 lightColor, float attenStartRange, float attenEndRange,
	float cosHalfInnerConeAngle, float cosHalfOuterConeAngle, float3 dirToViewer, float3 position, float3 normal,
	float3 diffuseAlbedo, float3 specularAlbedo, float shininess)
{
	float3 dirToLight = lightPos - position;
	float distToLight = length(dirToLight);
	dirToLight *= rcp(distToLight);

	float3 lightContrib = CalcLighting(dirToViewer, dirToLight, lightColor, normal, diffuseAlbedo, specularAlbedo, shininess);
	float distAtten = CalcLightDistanceAttenuation(distToLight, attenStartRange, attenEndRange);
	
	float cosDirAngle = dot(lightDir, -dirToLight);
	float dirAtten = CalcLightDirectionAttenuation(cosDirAngle, cosHalfInnerConeAngle, cosHalfOuterConeAngle);

	return (distAtten * dirAtten) * lightContrib;
}

float3 CalcDirectionalLightContribution(float3 lightDir, float3 lightColor, float3 dirToViewer, float3 normal,
	float3 diffuseAlbedo, float3 specularAlbedo, float shininess)
{
	float3 dirToLight = -lightDir;
	float3 lightContrib = CalcLighting(dirToViewer, dirToLight, lightColor, normal, diffuseAlbedo, specularAlbedo, shininess);

	return lightContrib;
}

#endif // __LIGHTING__