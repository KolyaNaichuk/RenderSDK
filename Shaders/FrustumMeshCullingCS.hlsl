#include "Foundation.hlsl"
#include "OverlapTest.hlsl"

struct MeshInstanceRange
{
	uint instanceOffset;
	uint numInstances;
	uint meshIndex;
};

cbuffer AppDataBuffer : register(b0)
{
	AppData g_AppData;
}

StructuredBuffer<MeshInstanceRange> g_MeshInstanceRangeBuffer : register(t0);
StructuredBuffer<AABB> g_InstanceWorldAABBBuffer : register(t1);

RWBuffer<uint> g_NumVisibleMeshesBuffer : register(u0);
RWBuffer<uint> g_NumVisibleInstancesBuffer : register(u1);
RWStructuredBuffer<MeshInstanceRange> g_VisibleInstanceRangeBuffer : register(u2);
RWBuffer<uint> g_VisibleInstanceIndexBuffer : register(u3);

groupshared uint g_NumVisibleInstancesPerMesh;
groupshared uint g_VisibleInstanceIndicesPerMesh[MAX_NUM_INSTANCES_PER_MESH];
groupshared uint g_VisibleInstanceOffsetPerMesh;

[numthreads(NUM_THREADS_PER_MESH, 1, 1)]
void Main(uint3 groupId : SV_GroupID, uint localIndex : SV_GroupIndex)
{
	if (localIndex == 0)
	{
		g_NumVisibleInstancesPerMesh = 0;
		g_VisibleInstanceOffsetPerMesh = 0;
	}
	GroupMemoryBarrierWithGroupSync();
 
	MeshInstanceRange meshInstanceRange = g_MeshInstanceRangeBuffer[groupId.x];
	for (uint index = localIndex; index < meshInstanceRange.numInstances; index += NUM_THREADS_PER_MESH)
	{
		uint instanceIndex = meshInstanceRange.instanceOffset + index;
		if (TestAABBAgainstFrustum(g_AppData.cameraWorldFrustumPlanes, g_InstanceWorldAABBBuffer[instanceIndex]))
		{
			uint listIndex;
			InterlockedAdd(g_NumVisibleInstancesPerMesh, 1, listIndex);
			g_VisibleInstanceIndicesPerMesh[listIndex] = instanceIndex;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	if (localIndex == 0)
	{
		if (g_NumVisibleInstancesPerMesh > 0)
		{
			uint instanceOffset;
			InterlockedAdd(g_NumVisibleInstancesBuffer[0], g_NumVisibleInstancesPerMesh, instanceOffset);
			g_VisibleInstanceOffsetPerMesh = instanceOffset;

			uint meshOffset;
			InterlockedAdd(g_NumVisibleMeshesBuffer[0], 1, meshOffset);
			g_VisibleInstanceRangeBuffer[meshOffset].instanceOffset = instanceOffset;
			g_VisibleInstanceRangeBuffer[meshOffset].numInstances = g_NumVisibleInstancesPerMesh;
			g_VisibleInstanceRangeBuffer[meshOffset].meshIndex = meshInstanceRange.meshIndex;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localIndex; index < g_NumVisibleInstancesPerMesh; index += NUM_THREADS_PER_MESH)
		g_VisibleInstanceIndexBuffer[g_VisibleInstanceOffsetPerMesh + index] = g_VisibleInstanceIndicesPerMesh[index];
}