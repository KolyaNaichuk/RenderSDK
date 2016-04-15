#include "OverlapTest.hlsl"

struct MeshData
{
	uint indexCount;
	uint startIndexLocation;
	uint startVertexLocation;
};

struct DrawIndexedArgs
{
	uint indexCountPerInstance;
	uint instanceCount;
	uint startIndexLocation;
	uint baseVertexLocation;
	uint startInstanceLocation;
};

struct DrawCommand
{
	uint meshIndexRootConstant;
	DrawIndexedArgs drawArgs;
};

struct CullingData
{
	float4 worldSpaceViewFrustumPlanes[6];
	uint numMeshes;
	uint3 notUsed1;
	float4 notUsed2[14];
};

cbuffer CullingDataBuffer : register(b0)
{
	CullingData g_CullingData;
};

StructuredBuffer<AABB> g_MeshBoundsBuffer : register(t0);
StructuredBuffer<MeshData> g_MeshDataBuffer : register(t1);
RWStructuredBuffer<DrawCommand> g_CommandBuffer : register(u0);
RWBuffer<uint> g_CommandCountBuffer : register(u1);

groupshared uint g_NumVisibleMeshes;
groupshared uint g_VisibleMeshIndices[THREAD_GROUP_SIZE];
groupshared uint g_OutArgsOffset;

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void Main(uint3 groupId : SV_GroupID, uint localIndex : SV_GroupIndex)
{
	if (localIndex == 0)
	{
		g_NumVisibleMeshes = 0;
		g_OutArgsOffset = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	uint globalIndex = groupId.x * THREAD_GROUP_SIZE + localIndex;
	if (globalIndex < g_CullingData.numMeshes)
	{
		if (TestAABBAgainstFrustum(g_CullingData.worldSpaceViewFrustumPlanes, g_MeshBoundsBuffer[globalIndex]))
		{
			uint listIndex;
			InterlockedAdd(g_NumVisibleMeshes, 1, listIndex);
			g_VisibleMeshIndices[listIndex] = globalIndex;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	if (localIndex == 0)
	{
		uint argsOffset;
		InterlockedAdd(g_CommandCountBuffer[0], g_NumVisibleMeshes, argsOffset);

		g_OutArgsOffset = argsOffset;
	}
	GroupMemoryBarrierWithGroupSync();

	if (localIndex < g_NumVisibleMeshes)
	{
		uint argsIndex = localIndex + g_OutArgsOffset;
		uint meshIndex = g_VisibleMeshIndices[localIndex];

		g_CommandBuffer[argsIndex].meshIndexRootConstant = meshIndex;
		g_CommandBuffer[argsIndex].drawArgs.indexCountPerInstance = g_MeshDataBuffer[meshIndex].indexCount;
		g_CommandBuffer[argsIndex].drawArgs.instanceCount = 1;
		g_CommandBuffer[argsIndex].drawArgs.startIndexLocation = g_MeshDataBuffer[meshIndex].startIndexLocation;
		g_CommandBuffer[argsIndex].drawArgs.baseVertexLocation = g_MeshDataBuffer[meshIndex].startVertexLocation;
		g_CommandBuffer[argsIndex].drawArgs.startInstanceLocation = 0;
	}
}