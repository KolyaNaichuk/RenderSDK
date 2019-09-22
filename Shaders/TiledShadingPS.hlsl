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
StructuredBuffer<SpotLightProps> g_SpotLightPropsBuffer : register(t5);
Buffer<uint> g_SpotLightIndexPerTileBuffer : register(t6);
StructuredBuffer<Range> g_SpotLightRangePerTileBuffer : register(t7);
Texture2DArray<float> g_SpotLightShadowMaps : register(t8);
#endif // ENABLE_SPOT_LIGHTS

Buffer<uint> g_MaterialTextureIndicesBuffer : register(t9);
Texture2D g_MaterialTextures[NUM_MATERIAL_TEXTURES] : register(t10);

SamplerState g_AnisoSampler : register(s0);
SamplerState g_ShadowMapSampler : register(s1);

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
	
	uint materialTextureOffset = materialID * NUM_TEXTURES_PER_MATERIAL;
	uint baseColorTextureIndex = g_MaterialTextureIndicesBuffer[materialTextureOffset];
	uint metalnessTextureIndex = g_MaterialTextureIndicesBuffer[materialTextureOffset + 1];
	uint roughnessTextureIndex = g_MaterialTextureIndicesBuffer[materialTextureOffset + 2];

	Texture2D baseColorTexture = g_MaterialTextures[NonUniformResourceIndex(baseColorTextureIndex)];
	Texture2D metalnessTexture = g_MaterialTextures[NonUniformResourceIndex(metalnessTextureIndex)];
	Texture2D roughnessTexture = g_MaterialTextures[NonUniformResourceIndex(roughnessTextureIndex)];
	
	float3 baseColor = baseColorTexture.SampleGrad(g_AnisoSampler, texCoord, texCoordDX, texCoordDY).rgb;
	float metalness = metalnessTexture.SampleGrad(g_AnisoSampler, texCoord, texCoordDX, texCoordDY).r;
	float roughness = roughnessTexture.SampleGrad(g_AnisoSampler, texCoord, texCoordDX, texCoordDY).r;

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
		SpotLightProps lightProps = g_SpotLightPropsBuffer[lightIndex];

		float visibility = CalcSpotLightVisibility(g_SpotLightShadowMaps, lightProps.lightID,
			g_ShadowMapSampler, lightProps.viewProjMatrix, lightProps.viewNearPlane, lightProps.rcpViewClipRange,
			lightProps.negativeExpShadowMapConstant, worldSpacePos);

		float3 reflectedRadiance = CalcSpotLightContribution(visibility, lightProps.worldSpacePos,
			lightProps.worldSpaceDir, lightProps.radiantIntensity, lightProps.rcpSquaredRange,
			lightProps.angleFalloffScale, lightProps.angleFalloffOffset, worldSpacePos,
			worldSpaceNormal, worldSpaceDirToViewer, baseColor, metalness, roughness);
		
		spotLightsContrib += reflectedRadiance;
	}
#endif // ENABLE_SPOT_LIGHTS
	
#if ENABLE_DIRECTIONAL_LIGHT == 1
	float3 directionalLightContrib = CalcDirectionalLightContribution(
		g_AppData.worldSpaceDirToSun, g_AppData.irradiancePerpToSunDir,
		worldSpaceNormal, worldSpaceDirToViewer, baseColor, metalness, roughness);
#else // ENABLE_DIRECTIONAL_LIGHT
	float3 directionalLightContrib = 0.0f;
#endif // ENABLE_DIRECTIONAL_LIGHT
		
	return float4(spotLightsContrib + directionalLightContrib, 1.0f);
}