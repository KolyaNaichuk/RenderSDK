#ifndef __LIGHT_UTILS__
#define __LIGHT_UTILS__

#define SHADING_MODE_PHONG			1
#define SHADING_MODE_BLINN_PHONG	2

#define LIGHT_TYPE_SPOT				1
#define LIGHT_TYPE_DIRECTIONAL		2

#define CUBE_MAP_FACE_POS_X			0
#define CUBE_MAP_FACE_NEG_X			1
#define CUBE_MAP_FACE_POS_Y			2
#define CUBE_MAP_FACE_NEG_Y			3
#define CUBE_MAP_FACE_POS_Z			4
#define CUBE_MAP_FACE_NEG_Z			5
#define NUM_CUBE_MAP_FACES			6

struct SpotLightProps
{
	float3 color;
	float lightRange;
	float3 worldSpacePos;
	float cosHalfInnerConeAngle;
	float3 worldSpaceDir;
	float cosHalfOuterConeAngle;
	float viewNearPlane;
	float rcpViewClipRange;
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

float CalcLightDistanceAttenuation(float distToLight, float lightRange)
{
	return max(0.0f, 1.0f - (distToLight / lightRange));
}

float CalcLightDirectionAttenuation(float cosDirAngle, float cosHalfInnerConeAngle, float cosHalfOuterConeAngle)
{
	return smoothstep(cosHalfOuterConeAngle, cosHalfInnerConeAngle, cosDirAngle);
}

float3 CalcPointLightContribution(float3 lightPos, float3 lightColor, float lightRange, float3 dirToViewer,
	float3 position, float3 normal, float3 diffuseAlbedo, float3 specularAlbedo, float shininess)
{
	float3 dirToLight = lightPos - position;
	float distToLight = length(dirToLight);
	dirToLight *= rcp(distToLight);

	float3 lightContrib = CalcLighting(dirToViewer, dirToLight, lightColor, normal, diffuseAlbedo, specularAlbedo, shininess);
	float distAtten = CalcLightDistanceAttenuation(distToLight, lightRange);

	return distAtten * lightContrib;
}

float3 CalcSpotLightContribution(float3 lightPos, float3 lightDir, float3 lightColor, float lightRange,
	float cosHalfInnerConeAngle, float cosHalfOuterConeAngle, float3 dirToViewer, float3 position, float3 normal,
	float3 diffuseAlbedo, float3 specularAlbedo, float shininess)
{
	float3 dirToLight = lightPos - position;
	float distToLight = length(dirToLight);
	dirToLight *= rcp(distToLight);

	float3 lightContrib = CalcLighting(dirToViewer, dirToLight, lightColor, normal, diffuseAlbedo, specularAlbedo, shininess);
	float distAtten = CalcLightDistanceAttenuation(distToLight, lightRange);
	
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

uint DetectCubeMapFaceIndex(float3 cubeMapCenter, float3 pointToTest)
{
	float3 dirToPoint = normalize(pointToTest - cubeMapCenter);
	float3 absDirToPoint = abs(dirToPoint);
	float maxAxis = max(absDirToPoint.x, max(absDirToPoint.y, absDirToPoint.z));
	
	if (maxAxis == absDirToPoint.x)
		return (dirToPoint.x > 0.0f) ? CUBE_MAP_FACE_POS_X : CUBE_MAP_FACE_NEG_X;
	
	if (maxAxis == absDirToPoint.y)
		return (dirToPoint.y > 0.0f) ? CUBE_MAP_FACE_POS_Y : CUBE_MAP_FACE_NEG_Y;

	if (maxAxis == absDirToPoint.z)
		return (dirToPoint.z > 0.0f) ? CUBE_MAP_FACE_POS_Z : CUBE_MAP_FACE_NEG_Z;

	return NUM_CUBE_MAP_FACES;
}

#endif // __LIGHT_UTILS__