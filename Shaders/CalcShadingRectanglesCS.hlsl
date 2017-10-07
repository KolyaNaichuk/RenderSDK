#include "Foundation.hlsl"

groupshared uint2 g_ScreenMinPoints[NUM_MESH_TYPES];
groupshared uint2 g_ScreenMaxPoints[NUM_MESH_TYPES];

#define NUM_THREADS_PER_GROUP (NUM_THREADS_X * NUM_THREADS_Y)

cbuffer AppDataBuffer : register(b0)
{
	AppData g_AppData;
}

Texture2D<uint> g_MaterialIDTexture : register(t0);
Buffer<uint> g_MeshTypePerMaterialIDBuffer : register(t1);

RWStructuredBuffer<uint2> g_ShadingRectangleMinPointBuffer : register(u0);
RWStructuredBuffer<uint2> g_ShadingRectangleMaxPointBuffer : register(u1);

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, 1)]
void Main(uint3 globalThreadId : SV_DispatchThreadID, uint localThreadIndex : SV_GroupIndex)
{
	const uint MAX_UINT = 0xffffffff;
	const uint EMPTY_MATERIAL_ID = 0;

	for (uint index = localThreadIndex; index < NUM_MESH_TYPES; index += NUM_THREADS_PER_GROUP)
	{
		g_ScreenMinPoints[index] = uint2(MAX_UINT, MAX_UINT);
		g_ScreenMaxPoints[index] = uint2(0, 0);
	}
	GroupMemoryBarrierWithGroupSync();

	if ((globalThreadId.x < g_AppData.screenSize.x) && (globalThreadId.y < g_AppData.screenSize.y))
	{
		uint materialID = g_MaterialIDTexture[globalThreadId.xy];
		if (materialID != EMPTY_MATERIAL_ID)
		{
			uint meshType = g_MeshTypePerMaterialIDBuffer[materialID];

			InterlockedMin(g_ScreenMinPoints[meshType].x, globalThreadId.x);
			InterlockedMin(g_ScreenMinPoints[meshType].y, globalThreadId.y);

			InterlockedMax(g_ScreenMaxPoints[meshType].x, globalThreadId.x);
			InterlockedMax(g_ScreenMaxPoints[meshType].y, globalThreadId.y);
		}
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localThreadIndex; index < NUM_MESH_TYPES; index += NUM_THREADS_PER_GROUP)
	{
		InterlockedMin(g_ShadingRectangleMinPointBuffer[index].x, g_ScreenMinPoints[index].x);
		InterlockedMin(g_ShadingRectangleMinPointBuffer[index].y, g_ScreenMinPoints[index].y);

		InterlockedMax(g_ShadingRectangleMaxPointBuffer[index].x, g_ScreenMaxPoints[index].x);
		InterlockedMax(g_ShadingRectangleMaxPointBuffer[index].y, g_ScreenMaxPoints[index].y);
	}
}