#ifndef __SHADOW_UTILS__
#define __SHADOW_UTILS__

float CalcPointLightVisibility(Texture2D<float3> expShadowMap, float4x4 lightViewProjMatrix,
	float lightViewNearPlane, float lightRcpViewClipRange, float3 worldSpacePos)
{
	float4 lightClipSpacePos = mul(lightViewProjMatrix, float4(worldSpacePos, 1.0f));
	
	float3 lightPostWDivideProjSpacePos = lightClipSpacePos.xyz / lightClipSpacePos.w;
	float2 shadowMapCoords = float2(0.5f * (lightPostWDivideProjSpacePos.x + 1.0f), 0.5f * (1.0f - lightPostWDivideProjSpacePos.y));
		
	float lightSpaceDepth = lightClipSpacePos.w;
	float normalizedLightSpaceDepth = (lightSpaceDepth - lightViewNearPlane) * lightRcpViewClipRange;

	return 1.0f;
}

float CalcSpotLightVisibility(Texture2D<float3> expShadowMap, ShadowMapTile shadowMapTile,
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

		lightVisibility = 1.0f;
	}
	return lightVisibility;
}

#endif // __SHADOW_UTILS__