#include "OverlapTest.hlsl"

struct CullingData
{
	float4 viewFrustumPlanes[6];
	uint numMeshes;
	uint3 notUsed1;
	float4 notUsed2[9];
};

cbuffer CullingDataBuffer : register(b0)
{
	CullingData g_CullingData;
};

StructuredBuffer<AABB> g_MeshBoundsBuffer : register(t0);
RWBuffer<uint> g_NumVisibleMeshesBuffer : register(u0);
RWBuffer<uint> g_VisibleMeshIndexBuffer : register(u1);

groupshared uint g_NumVisibleMeshes;
groupshared uint g_VisibleMeshIndices[THREAD_GROUP_SIZE];
groupshared uint g_OutIndicesOffset;

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void Main(uint3 groupId : SV_GroupID, uint localIndex : SV_GroupIndex)
{
	if (localIndex == 0)
	{
		g_NumVisibleMeshes = 0;
		g_OutIndicesOffset = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	uint globalIndex = groupId.x * THREAD_GROUP_SIZE + localIndex;
	if (globalIndex < g_CullingData.numMeshes)
	{
		if (TestAABBAgainstFrustum(g_CullingData.viewFrustumPlanes, g_MeshBoundsBuffer[globalIndex]))
		{
			uint listIndex;
			InterlockedAdd(g_NumVisibleMeshes, 1, listIndex);
			
			g_VisibleMeshIndices[listIndex] = globalIndex;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	if (localIndex == 0)
	{
		uint outIndicesOffset;
		InterlockedAdd(g_NumVisibleMeshesBuffer[0], g_NumVisibleMeshes, outIndicesOffset);

		g_OutIndicesOffset = outIndicesOffset;
	}
	GroupMemoryBarrierWithGroupSync();

	if (localIndex < g_NumVisibleMeshes)
	{
		uint outIndex = localIndex + g_OutIndicesOffset;
		uint meshIndex = g_VisibleMeshIndices[localIndex];

		g_VisibleMeshIndexBuffer[outIndex] = meshIndex;
	}
}
