#define NUM_THREADS 	(TILE_SIZE >> 1)

Texture2D<float2> g_TiledVarianceShadowMap : register(t0);
StructuredBuffer<uint2> g_TileOffsetBuffer : register(t1);
RWTexture2D<float2> g_TiledVarianceShadowMapSAT : register(u0);

groupshared float2 g_SharedData[TILE_SIZE][TILE_SIZE];

void RowPrefixSum(uint3 localPos)
{
	[unroll]
	for (uint stride = 1; stride <= NUM_THREADS; stride <<= 1)
	{
		uint index = (localPos.x + 1) * (stride << 1) - 1;
		if (index < TILE_SIZE)
			g_SharedData[index][localPos.y] += g_SharedData[index - stride][localPos.y];

		GroupMemoryBarrierWithGroupSync();
	}

	[unroll]
	for (uint stride = NUM_THREADS >> 1; stride > 0; stride >>= 1)
	{
		uint index = (localPos.x + 1) * (stride << 1) - 1;
		if ((index + stride) < TILE_SIZE)
			g_SharedData[index + stride][localPos.y] += g_SharedData[index][localPos.y];

		GroupMemoryBarrierWithGroupSync();
	}
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(uint3 groupId : SV_GroupID, uint3 localPos : SV_GroupThreadID)
{
	const uint2 tileOffset = g_TileOffsetBuffer[groupId.x];
	
	const uint2 globalPos1 = tileOffset + localPos.xy;
	const uint2 globalPos2 = tileOffset + uint2(localPos.x + 1, localPos.y);

	g_SharedData[2 * localPos.x][localPos.y] = g_TiledVarianceShadowMap[globalPos1];
	g_SharedData[2 * localPos.x + 1][localPos.y] = g_TiledVarianceShadowMap[globalPos2];
	GroupMemoryBarrierWithGroupSync();

	RowPrefixSum(localPos);
	float sum1 = g_SharedData[2 * localPos.x][localPos.y];
	float sum2 = g_SharedData[2 * localPos.x + 1][localPos.y];
	GroupMemoryBarrierWithGroupSync();

	g_SharedData[localPos.y][2 * localPos.x] = sum1;
	g_SharedData[localPos.y][2 * localPos.x + 1] = sum2;
	GroupMemoryBarrierWithGroupSync();

	RowPrefixSum(localPos);
	g_TiledVarianceShadowMapSAT[globalPos1] = g_SharedData[2 * localPos.x][localPos.y];
	g_TiledVarianceShadowMapSAT[globalPos2] = g_SharedData[2 * localPos.x + 1][localPos.y];
}