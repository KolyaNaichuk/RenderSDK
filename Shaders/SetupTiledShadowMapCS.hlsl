#define LIGHT_TYPE_POINT		1
#define LIGHT_TYPE_SPOT			2
#define NUM_CUBE_MAP_FACES		6

struct ShadowMapData
{
	float2 tileTexSpaceSize;
	float2 notUsed[31];
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

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void Main(uint3 tileId : SV_DispatchThreadID)
{
	if (tileId.x < g_NumLightsBuffer[0])
	{
		uint lightIndex = g_LightIndexBuffer[tileId.x];

#if (LIGHT_TYPE == LIGHT_TYPE_POINT)
		uint tileIndex = lightIndex * NUM_CUBE_MAP_FACES + tileId.y;
#endif // (LIGHT_TYPE == LIGHT_TYPE_POINT)

#if (LIGHT_TYPE == LIGHT_TYPE_SPOT)
		uint tileIndex = lightIndex;
#endif // (LIGHT_TYPE == LIGHT_TYPE_SPOT)

		ShadowMapTile tile;
		tile.texSpaceSize = g_ShadowMapData.tileTexSpaceSize;
		tile.texSpaceTopLeftPos = float2(tileId.xy) * tile.texSpaceSize;

		g_ShadowMapTileBuffer[tileIndex] = tile;

		matrix lightViewProjMatrix = g_LightViewProjMatrixBuffer[tileIndex];
		matrix shadowMapTileProjMatrix = 
		{
			tile.texSpaceSize.x, 0.0f, 0.0f, 0.0f,
			0.0f, tile.texSpaceSize.y, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			tile.texSpaceSize.x + 2.0f * tile.texSpaceTopLeftPos.x - 1.0f, 1.0f - tile.texSpaceSize.y - 2.0f * tile.texSpaceTopLeftPos.y, 0.0f, 1.0f
		};
		
		//Matrix is expected to be stored in column-major order
		//matrix lightViewProjTileMatrix = mul(shadowMapTileProjMatrix, lightViewProjMatrix);
		g_LightViewProjTileMatrixBuffer[tileIndex] = transpose(lightViewProjTileMatrix);
	}
}