#define GROUP_SIZE (SHADOW_MAP_TILE_SIZE / 2)

cbuffer Constants32BitBuffer : register(b0)
{
	uint2 g_TileTopLeftInPixels;
};

Texture2D<float2> g_TiledShadowMap : register(t0);
RWTexture2D<float2> g_TiledShadowMapSAT : register(u0);

groupshared float2 g_SharedData[SHADOW_MAP_TILE_SIZE];

[numthreads(GROUP_SIZE, 1, 1)]
void Main(uint3 localThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID)
{
	uint2 localPixelPos = uint2(localThreadId.x, groupId.y);

	g_SharedData[2 * localThreadId.x] = g_TiledShadowMap[g_TileTopLeftInPixels + localPixelPos];
	g_SharedData[2 * localThreadId.x + 1] = g_TiledShadowMap[g_TileTopLeftInPixels + uint2(localPixelPos.x + 1, localPixelPos.y)];
	GroupMemoryBarrierWithGroupSync();

	[unroll]
	for (uint stride = 1; stride <= GROUP_SIZE; stride <<= 1)
	{
		uint index = (localThreadId.x + 1) * (stride << 1) - 1;
		if (index < SHADOW_MAP_TILE_SIZE)
			g_SharedData[index] += g_SharedData[index - stride];

		GroupMemoryBarrierWithGroupSync();
	}

	[unroll]
	for (uint stride = GROUP_SIZE >> 1; stride > 0; stride >>= 1)
	{
		uint index = (localThreadId.x + 1) * (stride << 1) - 1;
		if ((index + stride) < SHADOW_MAP_TILE_SIZE)
			g_SharedData[index + stride] += g_SharedData[index];

		GroupMemoryBarrierWithGroupSync();
	}

	g_TiledShadowMapSAT[g_TileTopLeftInPixels + localPixelPos.yx] = g_SharedData[2 * localThreadId.x];
	g_TiledShadowMapSAT[g_TileTopLeftInPixels + uint2(localPixelPos.y, localPixelPos.x + 1)] = g_SharedData[2 * localThreadId.x + 1];
}