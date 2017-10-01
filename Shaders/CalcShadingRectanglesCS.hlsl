#include "Foundation.hlsl"

groupshared uint2 g_ScreenMinPoints[NUM_MESH_TYPES];
groupshared uint2 g_ScreenMaxPoints[NUM_MESH_TYPES];

#define NUM_THREADS_PER_GROUP (NUM_THREADS_X * NUM_THREADS_Y)

cbuffer AppDataBuffer : register(b0)
{
	AppData g_AppData;
}
Texture2D<uint> g_MeshTypeDepthTexture : register(t0);
RWStructuredBuffer<uint4> g_ShadingRectangleBuffer : register(u0);

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, 1)]
void Main(uint3 globalThreadId : SV_DispatchThreadID, uint localThreadIndex : SV_GroupIndex)
{
	for (uint index = localThreadIndex; index < NUM_MESH_TYPES; index += NUM_THREADS_PER_GROUP)
	{
		g_ScreenMinPoints[index] = uint2(0xffffffff, 0xffffffff);
		g_ScreenMaxPoints[index] = uint2(0, 0);
	}
	GroupMemoryBarrierWithGroupSync();

	if ((globalThreadId.x < g_AppData.screenSize.x) && (globalThreadId.y < g_AppData.screenSize.y))
	{
		uint meshType = g_MeshTypeDepthTexture[globalThreadId.xy].x;
		
		InterlockedMin(g_ScreenMinPoints[meshType].x, globalThreadId.x);
		InterlockedMin(g_ScreenMinPoints[meshType].y, globalThreadId.y);
		InterlockedMax(g_ScreenMaxPoints[meshType].x, globalThreadId.x);
		InterlockedMax(g_ScreenMaxPoints[meshType].y, globalThreadId.y);
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localThreadIndex; index < NUM_MESH_TYPES; index += NUM_THREADS_PER_GROUP)
	{
		InterlockedMin(g_ShadingRectangleBuffer[index].x, g_ScreenMinPoints[index].x);
		InterlockedMin(g_ShadingRectangleBuffer[index].y, g_ScreenMinPoints[index].y);
		InterlockedMax(g_ShadingRectangleBuffer[index].z, g_ScreenMaxPoints[index].x);
		InterlockedMax(g_ShadingRectangleBuffer[index].w, g_ScreenMaxPoints[index].y);
	}
}