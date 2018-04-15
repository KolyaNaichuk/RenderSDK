#define NUM_THREADS_X 	(TILE_SIZE >> 1)
#define NUM_THREADS_Y   (TILE_SIZE)

Texture2D<float2> g_TiledVarianceShadowMap : register(t0);
StructuredBuffer<uint2> g_TileOffsetBuffer : register(t1);
RWTexture2D<float2> g_TiledVarianceShadowMapSAT : register(u0);

groupshared float2 g_SharedData[TILE_SIZE][TILE_SIZE];

void RowPrefixSum(uint2 localThreadId)
{
	[unroll]
	for (uint stride = 1; stride <= NUM_THREADS_X; stride <<= 1)
	{
		uint index = (localThreadId.x + 1) * (stride << 1) - 1;
		if (index < TILE_SIZE)
			g_SharedData[index][localThreadId.y] += g_SharedData[index - stride][localThreadId.y];

		GroupMemoryBarrierWithGroupSync();
	}

	[unroll]
	for (uint stride = NUM_THREADS_X >> 1; stride > 0; stride >>= 1)
	{
		uint index = (localThreadId.x + 1) * (stride << 1) - 1;
		if ((index + stride) < TILE_SIZE)
			g_SharedData[index + stride][localThreadId.y] += g_SharedData[index][localThreadId.y];

		GroupMemoryBarrierWithGroupSync();
	}
}

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, 1)]
void Main(uint3 groupId : SV_GroupID, uint3 localThreadId : SV_GroupThreadID)
{
	const uint2 globalOffset = g_TileOffsetBuffer[groupId.x];

	const uint2 localPos1 = uint2(2 * localThreadId.x, localThreadId.y);
	const uint2 localPos2 = uint2(2 * localThreadId.x + 1, localThreadId.y);
	
	const uint2 globalPos1 = globalOffset + localPos1;
	const uint2 globalPos2 = globalOffset + localPos2;

	g_SharedData[localPos1.x][localPos1.y] = g_TiledVarianceShadowMap[globalPos1];
	g_SharedData[localPos2.x][localPos2.y] = g_TiledVarianceShadowMap[globalPos2];
	GroupMemoryBarrierWithGroupSync();

	RowPrefixSum(localThreadId.xy);
	float sum1 = g_SharedData[localPos1.x][localPos1.y];
	float sum2 = g_SharedData[localPos2.x][localPos2.y];
	GroupMemoryBarrierWithGroupSync();

	g_SharedData[localPos1.y][localPos1.x] = sum1;
	g_SharedData[localPos2.y][localPos2.x] = sum2;
	GroupMemoryBarrierWithGroupSync();

	RowPrefixSum(localThreadId.xy);
	g_TiledVarianceShadowMapSAT[globalPos1] = g_SharedData[localPos1.x][localPos1.y];
	g_TiledVarianceShadowMapSAT[globalPos2] = g_SharedData[localPos2.x][localPos2.y];
}