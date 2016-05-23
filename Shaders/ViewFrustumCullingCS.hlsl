#include "OverlapTest.hlsl"

struct MeshDesc
{
	uint indexCount;
	uint startIndexLocation;
	int  baseVertexLocation;
	uint materialIndex;
};

struct DrawIndexedArgs
{
	uint indexCountPerInstance;
	uint instanceCount;
	uint startIndexLocation;
	int  baseVertexLocation;
	uint startInstanceLocation;
};

struct DrawCommand
{
	uint root32BitConstant;
	DrawIndexedArgs drawArgs;
};

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
StructuredBuffer<MeshDesc> g_MeshDescBuffer : register(t1);
RWBuffer<uint> g_NumDrawsBuffer : register(u0);
RWStructuredBuffer<DrawCommand> g_DrawCommandBuffer : register(u1);

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
		uint argsOffset;
		InterlockedAdd(g_NumDrawsBuffer[0], g_NumVisibleMeshes, argsOffset);

		g_OutArgsOffset = argsOffset;
	}
	GroupMemoryBarrierWithGroupSync();

	if (localIndex < g_NumVisibleMeshes)
	{
		uint argsIndex = localIndex + g_OutArgsOffset;
		uint meshIndex = g_VisibleMeshIndices[localIndex];

		g_DrawCommandBuffer[argsIndex].root32BitConstant = g_MeshDescBuffer[meshIndex].materialIndex;
		g_DrawCommandBuffer[argsIndex].drawArgs.indexCountPerInstance = g_MeshDescBuffer[meshIndex].indexCount;
		g_DrawCommandBuffer[argsIndex].drawArgs.instanceCount = 1;
		g_DrawCommandBuffer[argsIndex].drawArgs.startIndexLocation = g_MeshDescBuffer[meshIndex].startIndexLocation;
		g_DrawCommandBuffer[argsIndex].drawArgs.baseVertexLocation = g_MeshDescBuffer[meshIndex].baseVertexLocation;
		g_DrawCommandBuffer[argsIndex].drawArgs.startInstanceLocation = 0;
	}
}