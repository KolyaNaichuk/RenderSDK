#include "Foundation.hlsl"

struct DrawCommand
{
	uint instanceOffset;
	uint materialIndex;
	DrawIndexedArgs args;
};

Buffer<uint> g_VisibilityBuffer : register(t0);
Buffer<uint> g_InstanceIndexBuffer : register(t1);
StructuredBuffer<MeshInfo> g_MeshInfoBuffer : register(t2);

RWBuffer<uint> g_VisibleInstanceIndexBuffer : register(u0);
RWBuffer<uint> g_NumVisibleMeshesPerTypeBuffer : register(u1);
RWStructuredBuffer<DrawCommand> g_DrawCommandBuffer : register(u2);
RWBuffer<uint> g_NumOccludedInstancesBuffer : register(u3);
RWBuffer<uint> g_OccludedInstanceIndexBuffer : register(u4);

groupshared uint g_NumVisibleInstances;
groupshared uint g_VisibleInstanceIndices[MAX_NUM_INSTANCES];

groupshared uint g_NumOccludedInstances;
groupshared uint g_OccludedInstanceIndices[MAX_NUM_INSTANCES];
groupshared uint g_OccludedInstanceOffset;

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void Main(uint3 groupId : SV_GroupID, uint localIndex : SV_GroupIndex)
{
	if (localIndex == 0)
	{
		g_NumVisibleInstances = 0;
		g_NumOccludedInstances = 0;
		g_OccludedInstanceOffset = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	MeshInfo meshInfo = g_MeshInfoBuffer[groupId.x];
	for (uint index = localIndex; index < meshInfo.numInstances; index += THREAD_GROUP_SIZE)
	{
		uint instanceIndex = g_InstanceIndexBuffer[meshInfo.instanceOffset + index];
		if (g_VisibilityBuffer[instanceIndex] > 0)
		{
			uint listIndex;
			InterlockedAdd(g_NumVisibleInstances, 1, listIndex);
			g_VisibleInstanceIndices[listIndex] = instanceIndex;
		}
		else
		{
			uint listIndex;
			InterlockedAdd(g_NumOccludedInstances, 1, listIndex);
			g_OccludedInstanceIndices[listIndex] = instanceIndex;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	if (localIndex == 0)
	{
		if (g_NumVisibleInstances > 0)
		{
			uint commandOffset;
			InterlockedAdd(g_NumVisibleMeshesPerTypeBuffer[meshInfo.meshType], 1, commandOffset);
			commandOffset += meshInfo.meshTypeOffset;
			
			g_DrawCommandBuffer[commandOffset].instanceOffset = meshInfo.instanceOffset;
			g_DrawCommandBuffer[commandOffset].materialIndex = meshInfo.materialIndex;
			g_DrawCommandBuffer[commandOffset].args.indexCountPerInstance = meshInfo.indexCountPerInstance;
			g_DrawCommandBuffer[commandOffset].args.instanceCount = g_NumVisibleInstances;
			g_DrawCommandBuffer[commandOffset].args.startIndexLocation = meshInfo.startIndexLocation;
			g_DrawCommandBuffer[commandOffset].args.baseVertexLocation = meshInfo.baseVertexLocation;
			g_DrawCommandBuffer[commandOffset].args.startInstanceLocation = 0;
		}
		if (g_NumOccludedInstances > 0)
		{
			uint instanceOffset;
			InterlockedAdd(g_NumOccludedInstancesBuffer[0], g_NumOccludedInstances, instanceOffset);
			g_OccludedInstanceOffset = instanceOffset;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localIndex; index < g_NumVisibleInstances; index += THREAD_GROUP_SIZE)
		g_VisibleInstanceIndexBuffer[meshInfo.instanceOffset + index] = g_VisibleInstanceIndices[index];

	for (uint index = localIndex; index < g_NumOccludedInstances; index += THREAD_GROUP_SIZE)
		g_OccludedInstanceIndexBuffer[g_OccludedInstanceOffset + index] = g_OccludedInstanceIndices[index];
}
