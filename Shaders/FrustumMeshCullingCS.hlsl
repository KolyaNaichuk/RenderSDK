#include "Foundation.hlsl"
#include "OverlapTest.hlsl"

struct MeshInstanceRange
{
	uint instanceOffset;
	uint numInstances;
	uint meshIndex;
};

struct CullingData
{
	float4 frustumPlanes[6];
	float4 notUsed[10];
};

cbuffer CullingDataBuffer : register(b0)
{
	CullingData g_CullingData;
}

StructuredBuffer<MeshInstanceRange> g_MeshInstanceRangeBuffer : register(t0);
StructuredBuffer<AABB> g_InstanceAABBBuffer : register(t1);

RWStructuredBuffer<uint> g_NumVisibleMeshesBuffer : register(u0);
RWStructuredBuffer<MeshInstanceRange> g_VisibleInstanceRangeBuffer : register(u1);
RWStructuredBuffer<uint> g_VisibleInstanceIndexBuffer : register(u2);
RWStructuredBuffer<DrawIndexedArgs> g_DrawVisibleInstanceCommandBuffer : register(u3);

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
		if (TestAABBAgainstFrustum(g_CullingData.frustumPlanes, g_InstanceAABBBuffer[instanceIndex]))
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
			InterlockedAdd(g_DrawVisibleInstanceCommandBuffer[0].instanceCount, g_NumVisibleInstancesPerMesh, instanceOffset);
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