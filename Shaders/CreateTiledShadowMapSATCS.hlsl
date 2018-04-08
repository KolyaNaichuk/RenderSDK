cbuffer Constants32BitBuffer : register(b0)
{
	uint2 g_TileTopLeftInPixels;
};

Texture2D<float2> g_TiledVarianceShadowMap : register(t0);
RWTexture2D<float2> g_TiledVarianceShadowMapSAT : register(u0);

groupshared float2 g_SharedData[2 * NUM_THREADS_X][NUM_THREADS_Y];

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, 1)]
void Main(uint3 tileSpacePos : SV_DispatchThreadID, uint3 localPos : SV_GroupThreadID)
{	
	g_SharedData[2 * localPos.x][localPos.y] = g_TiledVarianceShadowMap[g_TileTopLeftInPixels + tileSpacePos.xy];
	g_SharedData[2 * localPos.x + 1][localPos.y] = g_TiledVarianceShadowMap[g_TileTopLeftInPixels + uint2(tileSpacePos.x + 1, tileSpacePos.y)];
	GroupMemoryBarrierWithGroupSync();

	[unroll]
	for (uint stride = 1; stride <= NUM_THREADS_X; stride <<= 1)
	{
		uint index = (localPos.x + 1) * (stride << 1) - 1;
		if (index < 2 * NUM_THREADS_X)
			g_SharedData[index][localPos.y] += g_SharedData[index - stride][localPos.y];

		GroupMemoryBarrierWithGroupSync();
	}

	[unroll]
	for (uint stride = NUM_THREADS_X >> 1; stride > 0; stride >>= 1)
	{
		uint index = (localPos.x + 1) * (stride << 1) - 1;
		if ((index + stride) < 2 * NUM_THREADS_X)
			g_SharedData[index + stride][localPos.y] += g_SharedData[index][localPos.y];

		GroupMemoryBarrierWithGroupSync();
	}
	
	g_TiledVarianceShadowMapSAT[g_TileTopLeftInPixels + tileSpacePos.yx] = g_SharedData[2 * localPos.x][localPos.y];
	g_TiledVarianceShadowMapSAT[g_TileTopLeftInPixels + uint2(tileSpacePos.y, tileSpacePos.x + 1)] = g_SharedData[2 * localPos.x + 1][localPos.y];
}