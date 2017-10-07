#include "Lighting.hlsl"
#include "OverlapTest.hlsl"
#include "Reconstruction.hlsl"
#include "SphericalHarmonics.hlsl"
#include "Foundation.hlsl"
#include "VoxelGrid.hlsl"

struct PSInput
{
	float4 screenSpacePos	: SV_Position;
	float2 texCoord			: TEXCOORD0;
};

cbuffer AppDataBuffer : register(b0)
{
	AppData g_AppData;
}

Texture2D<float> g_DepthTexture : register(t0);
Texture2D<float2> g_TexCoordTexture : register(t1);
Texture2D<float4> g_NormalTexture : register(t2);
Texture2D<uint> g_MaterialIDTexture : register(t3);
Buffer<uint> g_FirstResourceIndexPerMaterialIDBuffer : register(t4);

#if ENABLE_POINT_LIGHTS == 1
StructuredBuffer<Sphere> g_PointLightBoundsBuffer : register(t5);
StructuredBuffer<PointLightProps> g_PointLightPropsBuffer : register(t6);
Buffer<uint> g_PointLightIndexPerTileBuffer : register(t7);
StructuredBuffer<Range> g_PointLightRangePerTileBuffer : register(t8);
#endif // ENABLE_POINT_LIGHTS

#if ENABLE_SPOT_LIGHTS == 1
StructuredBuffer<Sphere> g_SpotLightBoundsBuffer : register(t9);
StructuredBuffer<SpotLightProps> g_SpotLightPropsBuffer : register(t10);
Buffer<uint> g_SpotLightIndexPerTileBuffer : register(t11);
StructuredBuffer<Range> g_SpotLightRangePerTileBuffer : register(t12);
#endif // ENABLE_SPOT_LIGHTS

#if ENABLE_INDIRECT_LIGHT == 1
Texture3D<float4> g_IntensityRCoeffsTexture : register(t13);
Texture3D<float4> g_IntensityGCoeffsTexture : register(t14);
Texture3D<float4> g_IntensityBCoeffsTexture : register(t15);
#endif // ENABLE_INDIRECT_LIGHT

Texture2D<float4> g_MaterialTextures[NUM_MATERIAL_TEXTURES] : register(t16);

SamplerState g_AnisoSampler : register(s0);
SamplerState g_LinearSampler : register(s1);

[earlydepthstencil]
float4 Main(PSInput input) : SV_Target
{
	uint2 texturePos = uint2(input.screenSpacePos.xy);

	float hardwareDepth = g_DepthTexture[texturePos].x;
	float2 texCoord = g_TexCoordTexture[texturePos].xy;
	float3 worldSpaceNormal = g_NormalTexture[texturePos].xyz;
	uint materialID = g_MaterialIDTexture[texturePos].x;
	
	uint firstTextureIndex = g_FirstResourceIndexPerMaterialIDBuffer[materialID];
	Texture2D<float4> diffuseTexture = g_MaterialTextures[NonUniformResourceIndex(firstTextureIndex)];
	Texture2D<float4> specularTexture = g_MaterialTextures[NonUniformResourceIndex(firstTextureIndex + 1)];
	Texture2D<float> shininessTexture = g_MaterialTextures[NonUniformResourceIndex(firstTextureIndex + 2)];
	
	float3 diffuseAlbedo = diffuseTexture.Sample(g_AnisoSampler, texCoord).rgb;
	float3 specularAlbedo = specularTexture.Sample(g_AnisoSampler, texCoord).rgb;
	float shininess = shininessTexture.Sample(g_AnisoSampler, texCoord).r;

	float3 worldSpacePos = ComputeWorldSpacePosition(input.texCoord, hardwareDepth, g_AppData.viewProjInvMatrix).xyz;
	float3 worldSpaceDirToViewer = normalize(g_AppData.cameraWorldSpacePos.xyz - worldSpacePos);
		
	uint2 tilePos = texturePos / g_AppData.screenTileSize;
	uint tileIndex = tilePos.y * g_AppData.numScreenTiles.x + tilePos.x;
	
	float3 pointLightsContrib = float3(0.0f, 0.0f, 0.0f);
#if ENABLE_POINT_LIGHTS == 1
	uint pointLightIndexPerTileStart = g_PointLightRangePerTileBuffer[tileIndex].start;
	uint pointLightIndexPerTileEnd = pointLightIndexPerTileStart + g_PointLightRangePerTileBuffer[tileIndex].length;

	float3 lightContrib = float3(0.0f, 0.0f, 0.0f);
	for (uint lightIndexPerTile = pointLightIndexPerTileStart; lightIndexPerTile < pointLightIndexPerTileEnd; ++lightIndexPerTile)
	{
		uint lightIndex = g_PointLightIndexPerTileBuffer[lightIndexPerTile];

		float3 worldSpaceLightPos = g_PointLightBoundsBuffer[lightIndex].center;
		float attenEndRange = g_PointLightBoundsBuffer[lightIndex].radius;
		float attenStartRange = g_PointLightPropsBuffer[lightIndex].attenStartRange;
		float3 lightColor = g_PointLightPropsBuffer[lightIndex].color;

		pointLightsContrib += CalcPointLightContribution(worldSpaceLightPos, lightColor, attenStartRange, attenEndRange,
			worldSpaceDirToViewer, worldSpacePos, worldSpaceNormal, diffuseAlbedo, specularAlbedo, shininess);
	}
#endif // ENABLE_POINT_LIGHTS

	float3 spotLightsContrib = float3(0.0f, 0.0f, 0.0f);
#if ENABLE_SPOT_LIGHTS == 1
	uint spotLightIndexPerTileStart = g_SpotLightRangePerTileBuffer[tileIndex].start;
	uint spotLightIndexPerTileEnd = spotLightIndexPerTileStart + g_SpotLightRangePerTileBuffer[tileIndex].length;

	for (uint lightIndexPerTile = spotLightIndexPerTileStart; lightIndexPerTile < spotLightIndexPerTileEnd; ++lightIndexPerTile)
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

		spotLightsContrib += CalcSpotLightContribution(worldSpaceLightPos, worldSpaceLightDir, lightColor, attenStartRange, attenEndRange,
			cosHalfInnerConeAngle, cosHalfOuterConeAngle, worldSpaceDirToViewer, worldSpacePos, worldSpaceNormal,
			diffuseAlbedo, specularAlbedo, shininess);
	}
#endif // ENABLE_SPOT_LIGHTS

#if ENABLE_DIRECTIONAL_LIGHT == 1
	float3 directionalLightContrib = CalcDirectionalLightContribution(g_AppData.sunWorldSpaceDir.xyz, g_AppData.sunLightColor, worldSpaceDirToViewer,
		worldSpaceNormal, diffuseAlbedo, specularAlbedo, shininess);
#else // ENABLE_DIRECTIONAL_LIGHT
	float3 directionalLightContrib = float3(0.0f, 0.0f, 0.0f);
#endif // ENABLE_DIRECTIONAL_LIGHT
	
	float3 directRadiance = pointLightsContrib + spotLightsContrib + directionalLightContrib;

#if ENABLE_INDIRECT_LIGHT == 1
	float3 gridSpacePos = worldSpacePos - g_GridConfigData.worldSpaceOrigin.xyz;
	float3 gridTexCoord = gridSpacePos * g_GridConfigData.rcpSize.xyz;
	gridTexCoord.y = 1.0f - gridTexCoord;

	SHSpectralCoeffs incidentIntensityCoeffs;
	incidentIntensityCoeffs.r = g_IntensityRCoeffsTexture.SampleLevel(g_LinearSampler, gridTexCoord, 0.0f);
	incidentIntensityCoeffs.g = g_IntensityGCoeffsTexture.SampleLevel(g_LinearSampler, gridTexCoord, 0.0f);
	incidentIntensityCoeffs.b = g_IntensityBCoeffsTexture.SampleLevel(g_LinearSampler, gridTexCoord, 0.0f);

	float4 cosineCoeffs = SHProjectClampedCosine(worldSpaceNormal);
	float3 incidentIrradiance;
	incidentIrradiance.r = dot(incidentIntensityCoeffs.r, cosineCoeffs);
	incidentIrradiance.g = dot(incidentIntensityCoeffs.g, cosineCoeffs);
	incidentIrradiance.b = dot(incidentIntensityCoeffs.b, cosineCoeffs);
	incidentIrradiance = max(incidentIrradiance, 0.0f);

	float3 diffuseBRDF = diffuseAlbedo * g_RcpPI;
	float3 indirectRadiance = diffuseBRDF * incidentIrradiance;
#else // ENABLE_INDIRECT_LIGHT
	float3 indirectRadiance = float3(0.0f, 0.0f, 0.0f);
#endif // ENABLE_INDIRECT_LIGHT

	return float4(directRadiance + indirectRadiance, 1.0f);
}