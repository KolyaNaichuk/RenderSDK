#ifndef __LIGHTING__
#define __LIGHTING__

float3 CalcPhongLighting(float3 dirToViewer, float3 dirToLight, float3 lightColor,
	float3 normal, float3 diffuseAlbedo, float3 specularAlbedo, float specularPower)
{
	float NdotL = saturate(dot(normal, dirToLight));
	float3 diffuseColor = NdotL * diffuseAlbedo * lightColor;

	float3 reflectDir = normalize(reflect(-dirToLight, normal));
	float RdotV = saturate(dot(reflectDir, dirToViewer));
	float3 specularColor = (NdotL * pow(RdotV, specularPower)) * specularAlbedo * lightColor;

	return (diffuseColor + specularColor);
}

float3 CalcBlinnPhongLighting(float3 dirToViewer, float3 dirToLight, float3 lightColor,
	float3 normal, float3 diffuseAlbedo, float3 specularAlbedo, float specularPower)
{
	float NdotL = saturate(dot(normal, dirToLight));
	float3 diffuseColor = NdotL * diffuseAlbedo * lightColor;

	float3 halfDir = normalize(dirToLight + dirToViewer);
	float HdotN = saturate(dot(halfDir, normal));
	float3 specularColor = (NdotL * pow(HdotN, specularPower)) * specularAlbedo * lightColor;

	return (diffuseColor + specularColor);
}

float3 CalcLighting(float3 dirToViewer, float3 dirToLight, float3 lightColor,
	float3 normal, float3 diffuseAlbedo, float3 specularAlbedo, float specularPower)
{
#ifdef PHONG_LIGHTING
	return CalcPhongLighting(dirToViewer, dirToLight, lightColor, normal, diffuseAlbedo, specularAlbedo, specularPower);
#endif // PHONG_LIGHTING

#ifdef BLINN_PHONG_LIGHTING
	return CalcBlinnPhongLighting(dirToViewer, dirToLight, lightColor, normal, diffuseAlbedo, specularAlbedo, specularPower);
#endif // BLINN_PHONG_LIGHTING
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
	float3 dirToViewer, float3 position, float3 normal, float3 diffuseAlbedo, float3 specularAlbedo, float specularPower)
{
	float3 dirToLight = lightPos - position;
	float distToLight = length(dirToLight);
	dirToLight *= rcp(distToLight);

	float3 lightContrib = CalcLighting(dirToViewer, dirToLight, lightColor, normal, diffuseAlbedo, specularAlbedo, specularPower);
	float distAtten = CalcLightDistanceAttenuation(distToLight, attenStartRange, attenEndRange);

	return distAtten * lightContrib;
}

float3 CalcSpotLightContribution(float3 lightPos, float3 lightDir, float3 lightColor, float attenStartRange, float attenEndRange,
	float cosHalfInnerConeAngle, float cosHalfOuterConeAngle, float3 dirToViewer, float3 position, float3 normal,
	float3 diffuseAlbedo, float3 specularAlbedo, float specularPower)
{
	float3 dirToLight = lightPos - position;
	float distToLight = length(dirToLight);
	dirToLight *= rcp(distToLight);

	float3 lightContrib = CalcLighting(dirToViewer, dirToLight, lightColor, normal, diffuseAlbedo, specularAlbedo, specularPower);
	float distAtten = CalcLightDistanceAttenuation(distToLight, attenStartRange, attenEndRange);
	
	float cosDirAngle = dot(lightDir, -dirToLight);
	float dirAtten = CalcLightDirectionAttenuation(cosDirAngle, cosHalfInnerConeAngle, cosHalfOuterConeAngle);

	return (distAtten * dirAtten) * lightContrib;
}

float3 CalcDirectionalLightContribution(float3 lightDir, float3 lightColor, float3 dirToViewer, float3 normal,
	float3 diffuseAlbedo, float3 specularAlbedo, float specularPower)
{
	float3 dirToLight = -lightDir;
	float3 lightContrib = CalcLighting(dirToViewer, dirToLight, lightColor, normal, diffuseAlbedo, specularAlbedo, specularPower);

	return lightContrib;
}

#endif // __LIGHTING__