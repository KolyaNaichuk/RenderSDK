struct ShadowMapData
{
	float sizeInPixels;
	float rcpSizeInPixels;
	float tileSizeInPixels;
	float rcpTileSizeInPixels;
	float notUsed[60];
};

struct ShadowMapTile
{
	float2 texSpaceTopLeftPos;
	float2 texSpaceSize;
};

cbuffer ShadowMapDataBuffer : register(b0)
{
	ShadowMapData g_ShadowMapData;
};

Buffer<uint> g_NumLightsBuffer : register(t0);
Buffer<uint> g_LightIndexBuffer : register(t1);
StructuredBuffer<matrix> g_LightViewProjMatrixBuffer : register(t2);

RWStructuredBuffer<ShadowMapTile> g_ShadowMapTileBuffer : register(u0);
RWStructuredBuffer<matrix> g_LightViewProjTileMatrixBuffer : register(u1);

[numthreads(NUM_TILES_X, NUM_TILES_Y, 1)]
void Main(uint3 tileId : SV_GroupThreadID, uint tileIndex : SV_GroupIndex)
{
	if (tileIndex < g_NumLightsBuffer[0])
	{
		uint lightIndex = g_LightIndexBuffer[tileIndex];
		matrix lightViewProjMatrix = g_LightViewProjMatrixBuffer[lightIndex];
		
		ShadowMapTile tile;
		tile.texSpaceSize = g_ShadowMapData.tileSizeInPixels * g_ShadowMapData.rcpSizeInPixels;
		tile.texSpaceTopLeftPos = float2(tileId.xy) * tile.texSpaceSize;
		
		g_ShadowMapTileBuffer[lightIndex] = tile;

		matrix shadowMapTileProjMatrix =
		{
			tile.texSpaceSize.x, 0.0f, 0.0f, 0.0f,
			0.0f, tile.texSpaceSize.y, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			tile.texSpaceSize.x + 2.0f * tile.texSpaceTopLeftPos.x - 1.0f, 1.0f - tile.texSpaceSize.y - 2.0f * tile.texSpaceTopLeftPos.y, 0.0f, 1.0f
		};
		g_LightViewProjTileMatrixBuffer[lightIndex] = lightViewProjMatrix * shadowMapTileProjMatrix;
	}
}