#include "BoundingVolumes.hlsl"
#include "Foundation.hlsl"
#include "LightUtils.hlsl"

struct PSInput
{
	float4 screenSpacePos		: SV_Position;
	float3 worldSpacePos		: POSITION;
	float3 worldSpaceNormal		: NORMAL;
	float2 texCoord				: TEXCOORD;
};

cbuffer AppDataBuffer : register(b0)
{
	AppData g_AppData;
}

cbuffer Constants32BitBuffer : register(b1)
{
	uint g_MaterialID;
}

RasterizerOrderedTexture3D<float4> g_VoxelReflectanceTexture : register(u0);

#if ENABLE_SPOT_LIGHTS == 1
StructuredBuffer<Sphere> g_SpotLightWorldBoundsBuffer : register(t0);
StructuredBuffer<SpotLightProps> g_SpotLightPropsBuffer : register(t1);
#endif // ENABLE_SPOT_LIGHTS

Buffer<uint> g_MaterialTextureIndicesBuffer : register(t2);
Texture2D g_MaterialTextures[NUM_MATERIAL_TEXTURES] : register(t3);

SamplerState g_AnisoSampler : register(s0);

float4 MovingAverage(float3 prevValue, float prevCount, float3 newValue)
{
	float newCount = prevCount + 1;
	return float4((prevCount * prevValue + newValue) / newCount, newCount);
}

#if ENABLE_POINT_LIGHTS == 1
float3 ComputeReflectedRadianceFromPointLights(float3 worldSpacePos, float3 worldSpaceNormal, float3 diffuseAlbedo)
{
	float3 reflectedRadiance = float3(0.0f, 0.0f, 0.0f);
	for (uint lightIndex = 0; lightIndex < g_NumPointLights; ++lightIndex)
	{
		float3 incomingRadiance = g_PointLightPropsBuffer[lightIndex].color;
		float attenStartRange = g_PointLightPropsBuffer[lightIndex].attenStartRange;
		float3 lightWorldSpacePos = g_PointLightWorldBoundsBuffer[lightIndex].center;
		float attenEndRange = g_PointLightWorldBoundsBuffer[lightIndex].radius;

		float3 worldSpaceDirToLight = lightWorldSpacePos - worldSpacePos;
		float distToLight = length(worldSpaceDirToLight);
		worldSpaceDirToLight *= rcp(distToLight);
		
		float NdotL = saturate(dot(worldSpaceNormal, worldSpaceDirToLight));
		float shadowFactor = 1.0f;
		float distAtten = CalcLightDistanceAttenuation(distToLight, attenStartRange, attenEndRange);

		reflectedRadiance += diffuseAlbedo * incomingRadiance * (NdotL * shadowFactor * distAtten);
	}
	return reflectedRadiance;
}
#endif // ENABLE_POINT_LIGHTS

#if ENABLE_SPOT_LIGHTS == 1
float3 ComputeReflectedRadianceFromSpotLights(float3 worldSpacePos, float3 worldSpaceNormal, float3 diffuseAlbedo)
{
	float3 reflectedRadiance = float3(0.0f, 0.0f, 0.0f);
	for (uint lightIndex = 0; lightIndex < g_NumSpotLights; ++lightIndex)
	{
		Sphere lightBounds = g_SpotLightWorldBoundsBuffer[lightIndex];
		float3 lightWorldSpaceDir = g_SpotLightPropsBuffer[lightIndex].worldSpaceDir;
		float3 lightWorldSpacePos = lightBounds.center - lightBounds.radius * worldSpaceLightDir;
		float attenEndRange = g_SpotLightPropsBuffer[lightIndex].attenEndRange;
		float attenStartRange = g_SpotLightPropsBuffer[lightIndex].attenStartRange;
		float3 incomingRadiance = g_SpotLightPropsBuffer[lightIndex].color;
		float cosHalfInnerConeAngle = g_SpotLightPropsBuffer[lightIndex].cosHalfInnerConeAngle;
		float cosHalfOuterConeAngle = g_SpotLightPropsBuffer[lightIndex].cosHalfOuterConeAngle;

		float3 worldSpaceDirToLight = lightWorldSpacePos - worldSpacePos;
		float distToLight = length(worldSpaceDirToLight);
		worldSpaceDirToLight *= rcp(distToLight);

		float NdotL = saturate(dot(worldSpaceNormal, worldSpaceDirToLight));
		float shadowFactor = 1.0f;
		float distAtten = CalcLightDistanceAttenuation(distToLight, attenStartRange, attenEndRange);

		float cosDirAngle = dot(lightWorldSpaceDir, -worldSpaceDirToLight);
		float dirAtten = CalcLightDirectionAttenuation(cosDirAngle, cosHalfInnerConeAngle, cosHalfOuterConeAngle);

		reflectedRadiance += diffuseAlbedo * incomingRadiance * (NdotL * shadowFactor * distAtten * dirAtten);
	}
	return reflectedRadiance;
}
#endif // ENABLE_SPOT_LIGHTS

#if ENABLE_DIRECTIONAL_LIGHT == 1
float3 ComputeReflectedRadianceFromDirectionalLight(float3 worldSpaceNormal, float3 diffuseAlbedo)
{
	float3 incomingRadiance = g_AppData.sunLightColor;
	float3 worldSpaceDirToLight = -g_AppData.sunWorldSpaceDir.xyz;
	float NdotL = saturate(dot(worldSpaceNormal, worldSpaceDirToLight));
	float shadowFactor = 1.0f;
	float3 reflectedRadiance = diffuseAlbedo * incomingRadiance * (NdotL * shadowFactor);

	return reflectedRadiance;
}
#endif // ENABLE_DIRECTIONAL_LIGHT

void Main(PSInput input)
{
	uint firstTextureIndex = g_FirstResourceIndexPerMaterialIDBuffer[g_MaterialID];
	Texture2D diffuseTexture = g_MaterialTextures[NonUniformResourceIndex(firstTextureIndex)];
	float3 diffuseAlbedo = diffuseTexture.Sample(g_AnisoSampler, input.texCoord).rgb;

	float3 worldSpaceNormal = normalize(input.worldSpaceNormal);
	
	float3 reflectedRadiance = float3(0.0f, 0.0f, 0.0f);
#if ENABLE_POINT_LIGHTS == 1
	reflectedRadiance += ComputeReflectedRadianceFromPointLights(input.worldSpacePos, worldSpaceNormal, diffuseAlbedo);
#endif // ENABLE_POINT_LIGHTS

#if ENABLE_SPOT_LIGHTS == 1
	reflectedRadiance += ComputeReflectedRadianceFromSpotLights(input.worldSpacePos, worldSpaceNormal, diffuseAlbedo);
#endif // ENABLE_SPOT_LIGHTS

#if ENABLE_DIRECTIONAL_LIGHT == 1
	reflectedRadiance += ComputeReflectedRadianceFromDirectionalLight(worldSpaceNormal, diffuseAlbedo);
#endif // ENABLE_DIRECTIONAL_LIGHT
	
	float3 gridSpacePos = input.worldSpacePos - g_AppData.voxelGridWorldMinPoint;
	int3 voxelPos = floor(gridSpacePos * g_AppData.voxelRcpSize);
	
	float4 prevReflectedRadiance = g_VoxelReflectanceTexture[voxelPos];
	g_VoxelReflectanceTexture[voxelPos] = MovingAverage(prevReflectedRadiance.rgb, prevReflectedRadiance.a, reflectedRadiance);
}
