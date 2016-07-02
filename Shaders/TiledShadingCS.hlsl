#include "Lighting.hlsl"
#include "OverlapTest.hlsl"
#include "Reconstruction.hlsl"
#include "Foundation.hlsl"

struct TiledShadingData
{
	float2 rcpScreenSize;
	float2 notUsed1;
	float3 worldSpaceLightDir;
	float  notUsed2;
	float3 lightColor;
	float  notUsed3;
	float3 worldSpaceCameraPos;
	float  notUsed4;
	matrix viewProjInvMatrix;
	matrix notUsed5[2];
};

#define NUM_THREADS_PER_TILE	(TILE_SIZE * TILE_SIZE)

cbuffer TiledShadingDataBuffer : register(b0)
{
	TiledShadingData g_ShadingData;
}

Texture2D g_DepthTexture : register(t0);
Texture2D g_NormalTexture : register(t1);
Texture2D g_DiffuseTexture : register(t2);
Texture2D g_SpecularTexture : register(t3);

#if ENABLE_POINT_LIGHTS == 1
StructuredBuffer<Sphere> g_PointLightBoundsBuffer : register(t4);
StructuredBuffer<PointLightProps> g_PointLightPropsBuffer : register(t5);
Buffer<uint> g_PointLightIndexPerTileBuffer : register(t6);
StructuredBuffer<Range> g_PointLightRangePerTileBuffer : register(t7);
#endif

#if ENABLE_SPOT_LIGHTS == 1
StructuredBuffer<Sphere> g_SpotLightBoundsBuffer : register(t8);
StructuredBuffer<SpotLightProps> g_SpotLightPropsBuffer : register(t9);
Buffer<uint> g_SpotLightIndexPerTileBuffer : register(t10);
StructuredBuffer<Range> g_SpotLightRangePerTileBuffer : register(t11);
#endif

RWTexture2D<float4> g_AccumLightTexture : register(u0);

#if ENABLE_POINT_LIGHTS == 1
float3 CalcPointLightsContribution(uint tileIndex, float3 worldSpaceDirToViewer, float3 worldSpacePos, float3 worldSpaceNormal,
	float3 diffuseAlbedo, float3 specularAlbedo, float specularPower)
{
	uint lightIndexPerTileStart = g_PointLightRangePerTileBuffer[tileIndex].start;
	uint lightIndexPerTileEnd = lightIndexPerTileStart + g_PointLightRangePerTileBuffer[tileIndex].length;

	float3 lightContrib = float3(0.0f, 0.0f, 0.0f);
	for (uint lightIndexPerTile = lightIndexPerTileStart; lightIndexPerTile < lightIndexPerTileEnd; ++lightIndexPerTile)
	{
		uint lightIndex = g_PointLightIndexPerTileBuffer[lightIndexPerTile];

		float3 worldSpaceLightPos = g_PointLightBoundsBuffer[lightIndex].center;
		float attenEndRange = g_PointLightBoundsBuffer[lightIndex].radius;
		float attenStartRange = g_PointLightPropsBuffer[lightIndex].attenStartRange;
		float3 lightColor = g_PointLightPropsBuffer[lightIndex].color;

		lightContrib += CalcPointLightContribution(worldSpaceLightPos, lightColor, attenStartRange, attenEndRange,
			worldSpaceDirToViewer, worldSpacePos, worldSpaceNormal, diffuseAlbedo, specularAlbedo, specularPower);
	}
	return lightContrib;
}
#endif

#if ENABLE_SPOT_LIGHTS == 1
float3 CalcSpotLightsContribution(uint tileIndex, float3 worldSpaceDirToViewer, float3 worldSpacePos, float3 worldSpaceNormal,
	float3 diffuseAlbedo, float3 specularAlbedo, float specularPower)
{
	uint lightIndexPerTileStart = g_SpotLightRangePerTileBuffer[tileIndex].start;
	uint lightIndexPerTileEnd = lightIndexPerTileStart + g_SpotLightRangePerTileBuffer[tileIndex].length;

	float3 lightContrib = float3(0.0f, 0.0f, 0.0f);
	for (uint lightIndexPerTile = lightIndexPerTileStart; lightIndexPerTile < lightIndexPerTileEnd; ++lightIndexPerTile)
	{
		uint lightIndex = g_SpotLightIndexPerTileBuffer[lightIndexPerTile];
		
		Sphere lightBounds = g_SpotLightBoundsBuffer[lightIndex];
		float3 worldSpaceLightDir = g_SpotLightPropsBuffer[lightIndex].worldSpaceDir;
		float3 worldSpaceLightPos = lightBounds.center - lightBounds.radius * worldSpaceLightDir;
		float attenEndRange = g_SpotLightPropsBuffer[lightIndex].attenEndRange;
		float attenStartRange = g_SpotLightPropsBuffer[lightIndex].attenStartRange;
		float3 lightColor = g_SpotLightPropsBuffer[lightIndex].color;
		float cosHalfInnerConeAngle = g_SpotLightPropsBuffer[lightIndex].cosHalfInnerConeAngle;
		float cosHalfOuterConeAngle = g_SpotLightPropsBuffer[lightIndex].cosHalfOuterConeAngle;

		lightContrib += CalcSpotLightContribution(worldSpaceLightPos, worldSpaceLightDir, lightColor, attenStartRange, attenEndRange,
			cosHalfInnerConeAngle, cosHalfOuterConeAngle, worldSpaceDirToViewer, worldSpacePos, worldSpaceNormal,
			diffuseAlbedo, specularAlbedo, specularPower);
	}
	return lightContrib;
}
#endif

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void Main(uint3 globalThreadId : SV_DispatchThreadID, uint3 tileId : SV_GroupID)
{
	float  hardwareDepth = g_DepthTexture[globalThreadId.xy].x;
	float2 texCoord = (float2(globalThreadId.xy) + 0.5f) * g_ShadingData.rcpScreenSize;
	float3 worldSpacePos = ComputeWorldSpacePosition(texCoord, hardwareDepth, g_ShadingData.viewProjInvMatrix).xyz;
	float3 worldSpaceDirToViewer = normalize(g_ShadingData.worldSpaceCameraPos - worldSpacePos);
	float3 worldSpaceNormal = g_NormalTexture[globalThreadId.xy].xyz;
	float3 diffuseAlbedo = g_DiffuseTexture[globalThreadId.xy].rgb;
	float4 specularAlbedo = g_SpecularTexture[globalThreadId.xy].rgba;

	uint tileIndex = tileId.y * NUM_TILES_X + tileId.x;

#if ENABLE_POINT_LIGHTS == 1
	float3 pointLightsContrib = CalcPointLightsContribution(tileIndex, worldSpaceDirToViewer, worldSpacePos, worldSpaceNormal, diffuseAlbedo, specularAlbedo.rgb, specularAlbedo.a);
#else
	float3 pointLightsContrib = float3(0.0f, 0.0f, 0.0f);
#endif

#if ENABLE_SPOT_LIGHTS == 1
	float3 spotLightsContrib = CalcSpotLightsContribution(tileIndex, worldSpaceDirToViewer, worldSpacePos, worldSpaceNormal, diffuseAlbedo, specularAlbedo.rgb, specularAlbedo.a);
#else
	float3 spotLightsContrib = float3(0.0f, 0.0f, 0.0f);
#endif

#if ENABLE_DIRECTIONAL_LIGHT == 1
	float3 directionalLightContrib = CalcDirectionalLightContribution(tileIndex, g_ShadingData.worldSpaceLightDir, g_ShadingData.lightColor, worldSpaceDirToViewer,
		worldSpaceNormal, diffuseAlbedo, specularAlbedo.rgb, specularAlbedo.a);
#else
	float3 directionalLightContrib = float3(0.0f, 0.0f, 0.0f);
#endif

	float3 accumLight = pointLightsContrib + spotLightsContrib + directionalLightContrib;
	g_AccumLightTexture[globalThreadId.xy] = float4(accumLight, 1.0f);
}
