#include "ShadowUtils.hlsl"

struct VSInput
{
	uint vertexId		: SV_VertexID;
	uint tileId			: SV_InstanceID;
};

struct VSOutput
{
	float4 clipSpacePos : SV_Position;
	uint tileId			: SV_InstanceID;
};

StructuredBuffer<ShadowMapTile> g_ShadowMapTileBuffer : register(t0);

VSOutput Main(VSInput input)
{
	ShadowMapTile shadowMapTile = g_ShadowMapTileBuffer[input.tileId];
	float2 texSpaceTopLeft = shadowMapTile.texSpaceTopLeft;
	float2 texSpaceBottomRight = shadowMapTile.texSpaceTopLeft + shadowMapTile.texSpaceSize;

	float2 texSpaceCorner = float2(texSpaceTopLeft.x, texSpaceBottomRight.y);
	if (input.vertexId == 1)
		texSpaceCorner = texSpaceTopLeft;
	else if (input.vertexId == 2)
		texSpaceCorner = texSpaceBottomRight;
	else if (input.vertexId == 3)
		texSpaceCorner = float2(texSpaceBottomRight.x, texSpaceTopLeft.y);
	
	VSOutput output;
	output.clipSpacePos = float4(2.0f * texSpaceCorner.x - 1.0f, 1.0f - 2.0f * texSpaceCorner.y, 0.0f, 1.0f);
	output.tileId = input.tileId;

	return output;
}