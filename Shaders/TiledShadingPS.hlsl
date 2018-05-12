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

#if ENABLE_SPOT_LIGHTS == 1
StructuredBuffer<Sphere> g_SpotLightWorldBoundsBuffer : register(t5);
StructuredBuffer<SpotLightProps> g_SpotLightPropsBuffer : register(t6);
Buffer<uint> g_SpotLightIndexPerTileBuffer : register(t7);
StructuredBuffer<Range> g_SpotLightRangePerTileBuffer : register(t8);
Texture2DArray<float> g_SpotLightShadowMaps : register(t9);
StructuredBuffer<float4x4> g_SpotLightViewProjMatrixBuffer : register(t10);
#endif // ENABLE_SPOT_LIGHTS

Buffer<uint> g_FirstResourceIndexPerMaterialIDBuffer : register(t11);
Texture2D g_MaterialTextures[NUM_MATERIAL_TEXTURES] : register(t12);
SamplerState g_AnisoSampler : register(s0);

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
				
		//float lightVisibility = CalcSpotLightVisibility(g_SpotLightTiledExpShadowMap, shadowMapTile,
		//	lightViewProjMatrix, lightViewNearPlane, lightRcpViewClipRange, worldSpacePos);
		float lightVisibility = 1.0f;

		spotLightsContrib += lightVisibility * lightContrib;
	}
#endif // ENABLE_SPOT_LIGHTS

#if ENABLE_DIRECTIONAL_LIGHT == 1
	float3 directionalLightContrib = CalcDirectionalLightContribution(g_AppData.sunWorldSpaceDir.xyz, g_AppData.sunLightColor, worldSpaceDirToViewer,
		worldSpaceNormal, diffuseAlbedo, specularAlbedo, shininess);
#else // ENABLE_DIRECTIONAL_LIGHT
	float3 directionalLightContrib = float3(0.0f, 0.0f, 0.0f);
#endif // ENABLE_DIRECTIONAL_LIGHT
	
	float3 directRadiance = spotLightsContrib + directionalLightContrib;
	float3 indirectRadiance = float3(0.0f, 0.0f, 0.0f);
	
	return float4(directRadiance + indirectRadiance, 1.0f);
}