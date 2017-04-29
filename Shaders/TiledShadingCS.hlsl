#include "Lighting.hlsl"
#include "OverlapTest.hlsl"
#include "Reconstruction.hlsl"
#include "Foundation.hlsl"
#include "VoxelGrid.hlsl"
#include "SphericalHarmonics.hlsl"

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

cbuffer TiledShadingDataBuffer : register(b0)
{
	TiledShadingData g_ShadingData;
}

cbuffer GridConfigDataBuffer : register(b1)
{
	GridConfig g_GridConfigData;
}

Texture2D g_DepthTexture : register(t0);
Texture2D g_NormalTexture : register(t1);
Texture2D g_DiffuseTexture : register(t2);
Texture2D g_SpecularTexture : register(t3);

Texture3D g_IntensityRCoeffsTexture : register(t4);
Texture3D g_IntensityGCoeffsTexture : register(t5);
Texture3D g_IntensityBCoeffsTexture : register(t6);

StructuredBuffer<Sphere> g_PointLightBoundsBuffer : register(t7);
StructuredBuffer<PointLightProps> g_PointLightPropsBuffer : register(t8);
Buffer<uint> g_PointLightIndexPerTileBuffer : register(t9);
StructuredBuffer<Range> g_PointLightRangePerTileBuffer : register(t10);

StructuredBuffer<Sphere> g_SpotLightBoundsBuffer : register(t11);
StructuredBuffer<SpotLightProps> g_SpotLightPropsBuffer : register(t12);
Buffer<uint> g_SpotLightIndexPerTileBuffer : register(t13);
StructuredBuffer<Range> g_SpotLightRangePerTileBuffer : register(t14);

SamplerState g_LinearSampler : register(s0);
RWTexture2D<float4> g_AccumLightTexture : register(u0);

//////////////////////////////////////////////

#define A  0.4f  					 // shoulderStrength
#define B  0.3f  					 // linearStrength
#define C  0.1f  					 // linearAngle
#define D  0.2f  					 // toeStrength
#define E  0.01f 					 // toeNumerator
#define F  0.3f  					 // toeDenominator
#define LINEAR_WHITE 11.2f 

float3 FilmicFunc(in float3 x)
{
	return ((x*(A*x + C*B) + D*E) / (x*(A*x + B) + D*F)) - E / F;
}

float3 TonemapFilmic(in float3 color)
{
	float3 numerator = FilmicFunc(color);
	float3 denominator = FilmicFunc(float3(LINEAR_WHITE, LINEAR_WHITE, LINEAR_WHITE));
	return numerator / denominator;
}

//////////////////////////////////////////////

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void Main(uint3 globalThreadId : SV_DispatchThreadID, uint3 tileId : SV_GroupID)
{
	float hardwareDepth = g_DepthTexture[globalThreadId.xy].x;
	float2 texCoord = (float2(globalThreadId.xy) + 0.5f) * g_ShadingData.rcpScreenSize;
	float3 worldSpacePos = ComputeWorldSpacePosition(texCoord, hardwareDepth, g_ShadingData.viewProjInvMatrix).xyz;
	float3 worldSpaceDirToViewer = normalize(g_ShadingData.worldSpaceCameraPos - worldSpacePos);
	float3 worldSpaceNormal = g_NormalTexture[globalThreadId.xy].xyz;
	float3 diffuseAlbedo = g_DiffuseTexture[globalThreadId.xy].rgb;
	float4 specularAlbedo = g_SpecularTexture[globalThreadId.xy].rgba;
	uint tileIndex = tileId.y * NUM_TILES_X + tileId.x;

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
			worldSpaceDirToViewer, worldSpacePos, worldSpaceNormal, diffuseAlbedo, specularAlbedo.rgb, specularAlbedo.a);
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
			diffuseAlbedo, specularAlbedo.rgb, specularAlbedo.a);
	}
#endif // ENABLE_SPOT_LIGHTS

#if ENABLE_DIRECTIONAL_LIGHT == 1
	float3 directionalLightContrib = CalcDirectionalLightContribution(g_ShadingData.worldSpaceLightDir, g_ShadingData.lightColor, worldSpaceDirToViewer,
		worldSpaceNormal, diffuseAlbedo, specularAlbedo.rgb, specularAlbedo.a);
#else // ENABLE_DIRECTIONAL_LIGHT
	float3 directionalLightContrib = float3(0.0f, 0.0f, 0.0f);
#endif // ENABLE_DIRECTIONAL_LIGHT
	
	float3 directRadiance = pointLightsContrib + spotLightsContrib + directionalLightContrib;

#if ENABLE_INDIRECT_LIGHT == 1
	float3 gridSpacePos = worldSpacePos - g_GridConfigData.worldSpaceOrigin.xyz;
	float3 gridTexCoord = gridSpacePos * g_GridConfigData.rcpSize.xyz;

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

	float3 combinedRadiance = directRadiance + indirectRadiance;
	
	// Kolya. Applying Hawar's tonemapper
	////////////////////////////////////////////////////////////////////
	// Perform filmic tone-mapping with constant exposure
	//const float exposure = 1.2f;
	//combinedRadiance *= exposure;
	//combinedRadiance = TonemapFilmic(combinedRadiance);
	////////////////////////////////////////////////////////////////////

	g_AccumLightTexture[globalThreadId.xy] = float4(combinedRadiance, 1.0f);
}