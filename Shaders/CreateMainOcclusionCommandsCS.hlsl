struct MeshInstanceRange
{
	uint instanceOffset;
	uint numInstances;
	uint meshIndex;
};

struct MeshInfo
{
	uint meshType;
	uint meshTypeOffset;
	uint materialIndex;
	uint indexCountPerInstance;
	uint startIndexLocation;
	int  baseVertexLocation;
};

struct DrawCommand
{
	uint instanceOffset;
	uint materialIndex;
	DrawIndexedArgs args;
};

StructuredBuffer<uint> g_VisibilityBuffer : register(t0);
StructuredBuffer<uint> g_InstanceIndexBuffer : register(t1);
StructuredBuffer<MeshInstanceRange> g_MeshInstanceRangeBuffer : register(t2);
StructuredBuffer<MeshInfo> g_MeshInfoBuffer : register(t3);

RWStructuredBuffer<uint> g_VisibleInstanceIndexBuffer : register(u0);
RWStructuredBuffer<uint> g_NumVisibleMeshesPerTypeBuffer : register(u1);
RWStructuredBuffer<DrawCommand> g_DrawVisibleInstanceCommandBuffer : register(u2);

RWStructuredBuffer<uint> g_OccludedInstanceIndexBuffer : register(u3);
RWStructuredBuffer<uint> g_NumOccludedMeshesBuffer : register(u4);
RWStructuredBuffer<MeshInstanceRange> g_OccludedInstanceRangeBuffer : register(u5);
RWStructuredBuffer<DrawCommand> g_DrawOccludedInstanceCommandBuffer : register(u6);

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

	MeshInstanceRange meshInstanceRange = g_MeshInstanceRangeBuffer[groupId.x];
	for (uint index = localIndex; index < meshInstanceRange.numInstances; index += THREAD_GROUP_SIZE)
	{
		uint instanceIndex = g_InstanceIndexBuffer[meshInstanceRange.instanceOffset + index];
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
			MeshInfo meshInfo = g_MeshInfoBuffer[meshInstanceRange.meshIndex];
			
			uint commandOffset;
			InterlockedAdd(g_NumVisibleMeshesPerTypeBuffer[meshInfo.meshType], 1, commandOffset);
			commandOffset += meshInfo.meshTypeOffset;
			
			g_DrawVisibleInstanceCommandBuffer[commandOffset].instanceOffset = meshInstanceRange.instanceOffset;
			g_DrawVisibleInstanceCommandBuffer[commandOffset].materialIndex = meshInfo.materialIndex;
			g_DrawVisibleInstanceCommandBuffer[commandOffset].args.indexCountPerInstance = meshInfo.indexCountPerInstance;
			g_DrawVisibleInstanceCommandBuffer[commandOffset].args.instanceCount = g_NumVisibleInstances;
			g_DrawVisibleInstanceCommandBuffer[commandOffset].args.startIndexLocation = meshInfo.startIndexLocation;
			g_DrawVisibleInstanceCommandBuffer[commandOffset].args.baseVertexLocation = meshInfo.baseVertexLocation;
			g_DrawVisibleInstanceCommandBuffer[commandOffset].args.startInstanceLocation = 0;
		}
		if (g_NumOccludedInstances > 0)
		{
			uint instanceOffset;
			InterlockedAdd(g_DrawOccludedInstanceCommandBuffer[0].args.instanceCount, g_NumOccludedInstances, instanceOffset);
			g_OccludedInstanceOffset = instanceOffset;

			uint meshOffset;
			InterlockedAdd(g_NumOccludedMeshesBuffer[0], 1, meshOffset);
			g_OccludedInstanceRangeBuffer[meshOffset].instanceOffset = instanceOffset;
			g_OccludedInstanceRangeBuffer[meshOffset].numInstances = g_NumOccludedInstances;
			g_OccludedInstanceRangeBuffer[meshOffset].meshIndex = meshInstanceRange.meshIndex;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localIndex; index < g_NumVisibleInstances; index += THREAD_GROUP_SIZE)
		g_VisibleInstanceIndexBuffer[meshInstanceRange.instanceOffset + index] = g_VisibleInstanceIndices[index];

	for (uint index = localIndex; index < g_NumOccludedInstances; index += THREAD_GROUP_SIZE)
		g_OccludedInstanceIndexBuffer[g_OccludedInstanceOffset + index] = g_OccludedInstanceIndices[index];
}
