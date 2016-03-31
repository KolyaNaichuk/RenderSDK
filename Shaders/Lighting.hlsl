#ifndef __LIGHTING__
#define __LIGHTING__

float3 CalcPhongLighting(float3 normal, float3 dirToViewer, float3 dirToLight,
	float3 lightColor, float3 diffuseAlbedo, float3 specularAlbedo, float specularPower)
{
	float NdotL = saturate(dot(normal, dirToLight));
	float3 diffuseColor = NdotL * diffuseAlbedo * lightColor;

	float3 reflectDir = normalize(reflect(-dirToLight, normal));
	float RdotV = saturate(dot(reflectDir, dirToViewer));
	float3 specularColor = (NdotL * pow(RdotV, specularPower)) * specularAlbedo * lightColor;

	return (diffuseColor + specularColor);
}

float3 CalcBlinnPhongLighting(float3 normal, float3 dirToViewer, float3 dirToLight,
	float3 lightColor, float3 diffuseAlbedo, float3 specularAlbedo, float specularPower)
{
	float NdotL = saturate(dot(normal, dirToLight));
	float3 diffuseColor = NdotL * diffuseAlbedo * lightColor;

	float3 halfDir = normalize(dirToLight + dirToViewer);
	float HdotN = saturate(dot(halfDir, normal));
	float3 specularColor = (NdotL * pow(HdotN, specularPower)) * specularAlbedo * lightColor;

	return (diffuseColor + specularColor);
}

float CalcLightDistanceAttenuation(float distToLight, float attenStartRange, float attenEndRange)
{
	return saturate((attenEndRange - distToLight) / (attenEndRange - attenStartRange));
}

float CalcLightDirectionAttenuation(float cosDirAngle, float cosHalfInnerConeAngle, float cosHalfOuterConeAngle)
{
	return smoothstep(cosHalfOuterConeAngle, cosHalfInnerConeAngle, cosDirAngle);
}

#endif // __LIGHTING__