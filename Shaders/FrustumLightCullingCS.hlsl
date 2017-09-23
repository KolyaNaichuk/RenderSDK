#include "OverlapTest.hlsl"
#include "Foundation.hlsl"

cbuffer AppDataBuffer : register(b0)
{
	AppData g_AppData;
};

StructuredBuffer<Sphere> g_LightWorldBoundsBuffer : register(t0);

RWBuffer<uint> g_NumVisibleLightsBuffer : register(u0);
RWBuffer<uint> g_VisibleLightIndexBuffer : register(u1);

groupshared uint g_NumVisibleLights;
groupshared uint g_VisibleLightIndices[THREAD_GROUP_SIZE];
groupshared uint g_LightIndicesOffset;

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void Main(uint3 groupId : SV_GroupID, uint localIndex : SV_GroupIndex)
{
	if (localIndex == 0)
	{
		g_NumVisibleLights = 0;
		g_LightIndicesOffset = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	uint globalIndex = groupId.x * THREAD_GROUP_SIZE + localIndex;
	if (globalIndex < NUM_TOTAL_LIGHTS)
	{
		if (TestSphereAgainstFrustum(g_AppData.cameraWorldFrustumPlanes, g_LightWorldBoundsBuffer[globalIndex]))
		{
			uint listIndex;
			InterlockedAdd(g_NumVisibleLights, 1, listIndex);
			g_VisibleLightIndices[listIndex] = globalIndex;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	if (localIndex == 0)
	{
		uint lightIndicesOffset;
		InterlockedAdd(g_NumVisibleLightsBuffer[0], g_NumVisibleLights, lightIndicesOffset);
		g_LightIndicesOffset = lightIndicesOffset;
	}
	GroupMemoryBarrierWithGroupSync();

	if (localIndex < g_NumVisibleLights)
		g_VisibleLightIndexBuffer[g_LightIndicesOffset + localIndex] = g_VisibleLightIndices[localIndex];
}
