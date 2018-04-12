#ifndef __SHADOW_UTILS__
#define __SHADOW_UTILS__

struct ShadowMapTile
{
	uint2 topLeftInPixels;
	uint sizeInPixels;
	float2 texSpaceTopLeft;
	float texSpaceSize;
};

float ChebyshevUpperBound(float2 moments, float t)
{
	float probability = (t <= moments.x);

	float variance = moments.y - moments.x * moments.x;	
	float delta = t - moments.x;
	float maxProbability = variance / (variance + delta * delta);
	
	return max(probability, maxProbability);
}

float VSM(SamplerState shadowSampler, Texture2D<float2> varianceShadowMap, float2 shadowMapCoords, float receiverDepth)
{
	//float2 moments = varianceShadowMap.SampleLevel(shadowSampler, shadowMapCoords, 0.0f).xy;
	//return ChebyshevUpperBound(moments, receiverDepth);
	return 1.0f;
}

float CalcPointLightVisibility(SamplerState shadowSampler, Texture2D<float2> varianceShadowMap,
	float4x4 lightViewProjMatrix, float lightViewNearPlane, float lightRcpViewClipRange, float3 worldSpacePos)
{
	float4 lightClipSpacePos = mul(lightViewProjMatrix, float4(worldSpacePos, 1.0f));
	
	float3 lightPostWDivideProjSpacePos = lightClipSpacePos.xyz / lightClipSpacePos.w;
	float2 shadowMapCoords = float2(0.5f * (lightPostWDivideProjSpacePos.x + 1.0f), 0.5f * (1.0f - lightPostWDivideProjSpacePos.y));
		
	float lightSpaceDepth = lightClipSpacePos.w;
	float normalizedLightSpaceDepth = (lightSpaceDepth - lightViewNearPlane) * lightRcpViewClipRange;

	return VSM(shadowSampler, varianceShadowMap, shadowMapCoords, normalizedLightSpaceDepth);
}

float CalcSpotLightVisibility(SamplerState shadowSampler, Texture2D<float2> varianceShadowMap, ShadowMapTile shadowMapTile,
	float4x4 lightViewProjMatrix, float lightViewNearPlane, float lightRcpViewClipRange, float3 worldSpacePos)
{
	float4 lightClipSpacePos = mul(lightViewProjMatrix, float4(worldSpacePos, 1.0f));

	float3 lightPostWDivideProjSpacePos = lightClipSpacePos.xyz / lightClipSpacePos.w;
	float2 shadowMapCoords = float2(0.5f * (lightPostWDivideProjSpacePos.x + 1.0f), 0.5f * (1.0f - lightPostWDivideProjSpacePos.y));

	float2 tileTopLeftCoords = shadowMapTile.texSpaceTopLeft;
	float2 tileBottomRightCoords = shadowMapTile.texSpaceTopLeft + shadowMapTile.texSpaceSize;
	
	float lightVisibility = 0.0f;
	if (all(tileTopLeftCoords < shadowMapCoords) && all(shadowMapCoords < tileBottomRightCoords))
	{
		float lightSpaceDepth = lightClipSpacePos.w;
		float normalizedLightSpaceDepth = (lightSpaceDepth - lightViewNearPlane) * lightRcpViewClipRange;

		lightVisibility = VSM(shadowSampler, varianceShadowMap, shadowMapCoords, normalizedLightSpaceDepth);
	}
	return lightVisibility;
}

#endif // __SHADOW_UTILS__