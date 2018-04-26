#if FILTER_FLOAT
Texture2D<float> g_TiledShadowMap : register(t0);
StructuredBuffer<uint2> g_TileOffsetBuffer : register(t1);
RWTexture2D<float> g_TiledShadowMapSAT : register(u0);

groupshared float g_SharedData[TILE_SIZE][TILE_SIZE];

[numthreads(TILE_SIZE, 1, 1)]
void Main(uint3 groupId : SV_GroupID, uint threadIndex : SV_GroupIndex)
{
	const uint2 globalOffset = g_TileOffsetBuffer[groupId.x];

	float rowPrefixSum = 0.0f;
	[unroll] for (uint x = 0; x < TILE_SIZE; ++x)
	{
		rowPrefixSum += g_TiledShadowMap[globalOffset + uint2(x, threadIndex)];
		g_SharedData[x][threadIndex] = rowPrefixSum;
	}
	GroupMemoryBarrierWithGroupSync();

	float columnPrefixSum = 0.0f;
	[unroll] for (uint y = 0; y < TILE_SIZE; ++y)
	{
		columnPrefixSum += g_SharedData[threadIndex][y];
		g_TiledShadowMapSAT[globalOffset + uint2(threadIndex, y)] = columnPrefixSum;
	}
}
#endif // FILTER_FLOAT

#if FILTER_FLOAT2
Texture2D<float2> g_TiledExpShadowMap : register(t0);
StructuredBuffer<uint2> g_TileOffsetBuffer : register(t1);
RWTexture2D<float2> g_TiledExpShadowMapSAT : register(u0);

groupshared float2 g_SharedData[TILE_SIZE][TILE_SIZE];

[numthreads(TILE_SIZE, 1, 1)]
void Main(uint3 groupId : SV_GroupID, uint threadIndex : SV_GroupIndex)
{
	const uint2 globalOffset = g_TileOffsetBuffer[groupId.x];
	
	float2 rowPrefixSum = float2(0.0f, 0.0f);
	[unroll] for (uint x = 0; x < TILE_SIZE; ++x)
	{
		rowPrefixSum += g_TiledExpShadowMap[globalOffset + uint2(x, threadIndex)];
		g_SharedData[x][threadIndex] = rowPrefixSum;
	}
	GroupMemoryBarrierWithGroupSync();

	float2 columnPrefixSum = float2(0.0f, 0.0f);
	[unroll] for (uint y = 0; y < TILE_SIZE; ++y)
	{
		columnPrefixSum += g_SharedData[threadIndex][y];
		g_TiledExpShadowMapSAT[globalOffset + uint2(threadIndex, y)] = columnPrefixSum;
	}
}
#endif // FILTER_FLOAT2