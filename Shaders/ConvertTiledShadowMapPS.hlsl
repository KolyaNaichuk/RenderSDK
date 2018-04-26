#include "LightUtils.hlsl"

struct PSInput
{
	float4 screenSpacePos	: SV_Position;
	uint tileId				: SV_InstanceID;
};

struct ConvertShadowMapParams
{
	float lightViewNearPlane;
	float lightRcpViewClipRange;
	float lightProjMatrix43;
	float lightProjMatrix33;
};

Texture2D<float> g_TiledShadowMap : register(t0);
StructuredBuffer<ConvertShadowMapParams> g_ConvertShadowMapParamsBuffer : register(t1);

float3 Main(PSInput input) : SV_Target
{
#if LIGHT_TYPE == LIGHT_TYPE_POINT
	uint lightIndex = input.tileId / NUM_CUBE_MAP_FACES;
#endif // LIGHT_TYPE_POINT

#if LIGHT_TYPE == LIGHT_TYPE_SPOT
	uint lightIndex = input.tileId;
#endif // LIGHT_TYPE_SPOT

	ConvertShadowMapParams params = g_ConvertShadowMapParamsBuffer[lightIndex];
	uint2 pixelPos = uint2(input.screenSpacePos.xy);

	float hardwareDepth = g_TiledShadowMap[pixelPos].r;
	float lightSpaceDepth = params.lightProjMatrix43 / (hardwareDepth - params.lightProjMatrix33);
	float normalizedLightSpaceDepth = (lightSpaceDepth - params.lightViewNearPlane) * params.lightRcpViewClipRange;

	return float3(normalizedLightSpaceDepth, 0.5f, 1.0f);
}
