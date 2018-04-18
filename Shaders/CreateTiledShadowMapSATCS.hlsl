Texture2D<float2> g_TiledVarianceShadowMap : register(t0);
StructuredBuffer<uint2> g_TileOffsetBuffer : register(t1);
RWTexture2D<float2> g_TiledVarianceShadowMapSAT : register(u0);

groupshared float2 g_SharedData[TILE_SIZE][TILE_SIZE];

[numthreads(TILE_SIZE, 1, 1)]
void Main(uint3 groupId : SV_GroupID, uint threadIndex : SV_GroupIndex)
{
	const uint2 globalOffset = g_TileOffsetBuffer[groupId.x];

	g_SharedData[0][threadIndex] = g_TiledVarianceShadowMap[globalOffset + uint2(0, threadIndex)];
	[unroll] for (uint x = 1; x < TILE_SIZE; ++x)
		g_SharedData[x][threadIndex] = g_SharedData[x - 1][threadIndex] + g_TiledVarianceShadowMap[globalOffset + uint2(x, threadIndex)];
	GroupMemoryBarrierWithGroupSync();

	[unroll] for (uint y = 1; y < TILE_SIZE; ++y)
		g_SharedData[threadIndex][y] += g_SharedData[threadIndex][y - 1];
	GroupMemoryBarrierWithGroupSync();

	[unroll] for (uint x = 0; x < TILE_SIZE; ++x)
		g_TiledVarianceShadowMapSAT[globalOffset + uint2(x, threadIndex)] = g_SharedData[x][threadIndex];
}