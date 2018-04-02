#include "Foundation.hlsl"
#include "EncodingUtils.hlsl"
#include "LightUtils.hlsl"
#include "ShadowUtils.hlsl"
#include "OverlapTest.hlsl"
#include "Reconstruction.hlsl"

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
Texture2D<float2> g_GBuffer1 : register(t1);
Texture2D<float2> g_GBuffer2 : register(t2);
Texture2D<uint2> g_GBuffer3 : register(t3);
Texture2D<float4> g_GBuffer4 : register(t4);
Buffer<uint> g_FirstResourceIndexPerMaterialIDBuffer : register(t5);

#if ENABLE_POINT_LIGHTS == 1
StructuredBuffer<Sphere> g_PointLightWorldBoundsBuffer : register(t6);
StructuredBuffer<PointLightProps> g_PointLightPropsBuffer : register(t7);
Buffer<uint> g_PointLightIndexPerTileBuffer : register(t8);
StructuredBuffer<Range> g_PointLightRangePerTileBuffer : register(t9);
Texture2D<float2> g_PointLightTiledVarianceShadowMap : register(t10);
StructuredBuffer<float4x4> g_PointLightViewProjMatrixBuffer : register(t11);
#endif // ENABLE_POINT_LIGHTS

#if ENABLE_SPOT_LIGHTS == 1
StructuredBuffer<Sphere> g_SpotLightWorldBoundsBuffer : register(t12);
StructuredBuffer<SpotLightProps> g_SpotLightPropsBuffer : register(t13);
Buffer<uint> g_SpotLightIndexPerTileBuffer : register(t14);
StructuredBuffer<Range> g_SpotLightRangePerTileBuffer : register(t15);
Texture2D<float2> g_SpotLightTiledVarianceShadowMap : register(t16);
StructuredBuffer<float4x4> g_SpotLightViewProjMatrixBuffer : register(t17);
StructuredBuffer<ShadowMapTile> g_SpotLightShadowMapTileBuffer : register(t18);
#endif // ENABLE_SPOT_LIGHTS

Texture2D g_MaterialTextures[NUM_MATERIAL_TEXTURES] : register(t19);

SamplerState g_AnisoSampler : register(s0);
SamplerState g_VarianceShadowMapSampler : register(s1);

[earlydepthstencil]
float4 Main(PSInput input) : SV_Target
{
	uint2 pixelPos = uint2(input.screenSpacePos.xy);

	float hardwareDepth = g_DepthTexture[pixelPos].x;
	float2 texCoord = g_GBuffer1[pixelPos].xy;
	float2 derivativesLength = g_GBuffer2[pixelPos].xy;
	uint encodedDerivativesRotation = g_GBuffer3[pixelPos].x;
	uint materialID = g_GBuffer3[pixelPos].y;
	float3 worldSpaceNormal = g_GBuffer4[pixelPos].xyz;
	
	float2 texCoordDX;
	float2 texCoordDY;
	DecodeTextureCoordinateDerivatives(derivativesLength, encodedDerivativesRotation, texCoordDX, texCoordDY);
	
	uint firstTextureIndex = g_FirstResourceIndexPerMaterialIDBuffer[materialID];
	Texture2D diffuseTexture = g_MaterialTextures[NonUniformResourceIndex(firstTextureIndex)];
	Texture2D specularTexture = g_MaterialTextures[NonUniformResourceIndex(firstTextureIndex + 1)];
	Texture2D shininessTexture = g_MaterialTextures[NonUniformResourceIndex(firstTextureIndex + 2)];
	
	float3 diffuseAlbedo = diffuseTexture.SampleGrad(g_AnisoSampler, texCoord, texCoordDX, texCoordDY).rgb;
	float3 specularAlbedo = specularTexture.SampleGrad(g_AnisoSampler, texCoord, texCoordDX, texCoordDY).rgb;
	float shininess = shininessTexture[uint2(0, 0)].r;

	float3 worldSpacePos = ComputeWorldSpacePosition(input.texCoord, hardwareDepth, g_AppData.viewProjInvMatrix).xyz;
	float3 worldSpaceDirToViewer = normalize(g_AppData.cameraWorldSpacePos.xyz - worldSpacePos);
		
	uint2 tilePos = pixelPos / g_AppData.screenTileSize;
	uint tileIndex = tilePos.y * g_AppData.numScreenTiles.x + tilePos.x;
	
	float3 pointLightsContrib = float3(0.0f, 0.0f, 0.0f);
#if ENABLE_POINT_LIGHTS == 1
	uint pointLightIndexPerTileStart = g_PointLightRangePerTileBuffer[tileIndex].start;
	uint pointLightIndexPerTileEnd = pointLightIndexPerTileStart + g_PointLightRangePerTileBuffer[tileIndex].length;

	for (uint lightIndexPerTile = pointLightIndexPerTileStart; lightIndexPerTile < pointLightIndexPerTileEnd; ++lightIndexPerTile)
	{
		uint lightIndex = g_PointLightIndexPerTileBuffer[lightIndexPerTile];

		float3 lightWorldSpacePos = g_PointLightWorldBoundsBuffer[lightIndex].center;
		float lightRange = g_PointLightWorldBoundsBuffer[lightIndex].radius;
		float3 lightColor = g_PointLightPropsBuffer[lightIndex].color;
		float lightViewNearPlane = g_PointLightPropsBuffer[lightIndex].viewNearPlane;
		float lightRcpViewClipRange = g_PointLightPropsBuffer[lightIndex].rcpViewClipRange;

		float3 lightContrib = CalcPointLightContribution(lightWorldSpacePos, lightColor, lightRange,
			worldSpaceDirToViewer, worldSpacePos, worldSpaceNormal, diffuseAlbedo, specularAlbedo, shininess);

		uint faceIndex = DetectCubeMapFaceIndex(lightWorldSpacePos, worldSpacePos);
		uint lightFrustumIndex = NUM_CUBE_MAP_FACES * lightIndex + faceIndex;
		float4x4 lightViewProjMatrix = g_PointLightViewProjMatrixBuffer[lightFrustumIndex];
		
		float lightVisibility = CalcPointLightVisibility(g_VarianceShadowMapSampler, g_PointLightTiledVarianceShadowMap,
			lightViewProjMatrix, lightViewNearPlane, lightRcpViewClipRange, worldSpacePos);
		
		pointLightsContrib += lightVisibility * lightContrib;
	}
#endif // ENABLE_POINT_LIGHTS

	float3 spotLightsContrib = float3(0.0f, 0.0f, 0.0f);
#if ENABLE_SPOT_LIGHTS == 1
	uint spotLightIndexPerTileStart = g_SpotLightRangePerTileBuffer[tileIndex].start;
	uint spotLightIndexPerTileEnd = spotLightIndexPerTileStart + g_SpotLightRangePerTileBuffer[tileIndex].length;

	for (uint lightIndexPerTile = spotLightIndexPerTileStart; lightIndexPerTile < spotLightIndexPerTileEnd; ++lightIndexPerTile)
	{
		uint lightIndex = g_SpotLightIndexPerTileBuffer[lightIndexPerTile];

		Sphere lightBounds = g_SpotLightWorldBoundsBuffer[lightIndex];
		float3 lightColor = g_SpotLightPropsBuffer[lightIndex].color;
		float3 lightWorldSpaceDir = g_SpotLightPropsBuffer[lightIndex].worldSpaceDir;
		float3 lightWorldSpacePos = lightBounds.center - lightBounds.radius * lightWorldSpaceDir;
		float lightRange = g_SpotLightPropsBuffer[lightIndex].lightRange;
		float cosHalfInnerConeAngle = g_SpotLightPropsBuffer[lightIndex].cosHalfInnerConeAngle;
		float cosHalfOuterConeAngle = g_SpotLightPropsBuffer[lightIndex].cosHalfOuterConeAngle;
		float lightViewNearPlane = g_SpotLightPropsBuffer[lightIndex].viewNearPlane;
		float lightRcpViewClipRange = g_SpotLightPropsBuffer[lightIndex].rcpViewClipRange;
		
		float3 lightContrib = CalcSpotLightContribution(lightWorldSpacePos, lightWorldSpaceDir, lightColor, lightRange,
			cosHalfInnerConeAngle, cosHalfOuterConeAngle, worldSpaceDirToViewer, worldSpacePos, worldSpaceNormal,
			diffuseAlbedo, specularAlbedo, shininess);

		float4x4 lightViewProjMatrix = g_SpotLightViewProjMatrixBuffer[lightIndex];
		ShadowMapTile shadowMapTile = g_SpotLightShadowMapTileBuffer[lightIndex];
		
		float lightVisibility = CalcSpotLightVisibility(g_VarianceShadowMapSampler, g_SpotLightTiledVarianceShadowMap, shadowMapTile,
			lightViewProjMatrix, lightViewNearPlane, lightRcpViewClipRange, worldSpacePos);

		spotLightsContrib += lightVisibility * lightContrib;
	}
#endif // ENABLE_SPOT_LIGHTS

#if ENABLE_DIRECTIONAL_LIGHT == 1
	float3 directionalLightContrib = CalcDirectionalLightContribution(g_AppData.sunWorldSpaceDir.xyz, g_AppData.sunLightColor, worldSpaceDirToViewer,
		worldSpaceNormal, diffuseAlbedo, specularAlbedo, shininess);
#else // ENABLE_DIRECTIONAL_LIGHT
	float3 directionalLightContrib = float3(0.0f, 0.0f, 0.0f);
#endif // ENABLE_DIRECTIONAL_LIGHT
	
	float3 directRadiance = pointLightsContrib + spotLightsContrib + directionalLightContrib;
	float3 indirectRadiance = float3(0.0f, 0.0f, 0.0f);
	
	return float4(directRadiance + indirectRadiance, 1.0f);
}