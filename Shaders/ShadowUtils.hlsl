#ifndef __SHADOW_UTILS__
#define __SHADOW_UTILS__

float CalcSpotLightVisibility(Texture2DArray<float> lightShadowMaps, uint shadowMapIndex,
	SamplerState shadowMapSampler, float4x4 lightViewProjMatrix, float lightViewNearPlane,
	float lightRcpViewClipRange, float negativeExpShadowMapConstant, float3 worldSpacePos)
{
	float4 lightClipSpacePos = mul(lightViewProjMatrix, float4(worldSpacePos, 1.0f));
	float lightSpaceDepth = lightClipSpacePos.w;
	float linearDepth = (lightSpaceDepth - lightViewNearPlane) * lightRcpViewClipRange;

	float3 lightPostWDivideProjSpacePos = lightClipSpacePos.xyz / lightClipSpacePos.w;
	float2 shadowMapCoords = float2(0.5f * (lightPostWDivideProjSpacePos.x + 1.0f), 0.5f * (1.0f - lightPostWDivideProjSpacePos.y));

	float shadowMapExpDepth = lightShadowMaps.SampleLevel(shadowMapSampler, float3(shadowMapCoords, shadowMapIndex), 0.0f);
	float lightVisibility = saturate(exp(negativeExpShadowMapConstant * linearDepth) * shadowMapExpDepth);
	
	return lightVisibility;
}

#endif // __SHADOW_UTILS__