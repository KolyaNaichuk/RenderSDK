#ifndef __SHADOW_UTILS__
#define __SHADOW_UTILS__

struct ShadowMapTile
{
	float2 texSpaceTopLeft;
	float texSpaceSize;
};

float CalcShadowContribution(float3 worldSpacePos, Texture2D tiledVarianceShadowMap, ShadowMapTile shadowMapTile,
	float4x4 lightViewProjMatrix, float lightViewNearPlane, float lightRcpViewClipRange)
{
	float shadowContrib = 0.0f;

	float4 lightClipSpacePos = mul(lightViewProjMatrix, float4(worldSpacePos, 1.0f));
	float3 lightPostWDivideProjSpacePos = lightClipSpacePos.xyz / lightClipSpacePos.w;
	float2 shadowMapCoords = float2(0.5f * (lightPostWDivideProjSpacePos.x + 1.0f), 0.5f * (1.0f - lightPostWDivideProjSpacePos.y));

	float2 tileTopLeftCoords = shadowMapTile.texSpaceTopLeft;
	float2 tileBottomRightCoords = shadowMapTile.texSpaceTopLeft + shadowMapTile.texSpaceSize;
	
	if (all(tileTopLeftCoords < shadowMapCoords) && all(shadowMapCoords < tileBottomRightCoords))
	{
		float lightSpaceDepth = lightClipSpacePos.w;
		float normalizedLightSpaceDepth = (lightSpaceDepth - lightViewNearPlane) * lightRcpViewClipRange;
	}

	return shadowContrib;
}

#endif // __SHADOW_UTILS__