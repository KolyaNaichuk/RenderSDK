struct InstanceRange
{
	uint instanceOffset;
	uint numInstances;
	uint meshIndex;
};

struct MeshInfo
{
	uint meshType;
	uint meshTypeOffset;
	uint materialId;
	uint indexCountPerInstance;
	uint startIndexLocation;
	int  baseVertexLocation;
};

struct DrawCommand
{
	uint instanceOffset;
	uint materialId;
	DrawIndexedArgs args;
};

StructuredBuffer<uint> g_VisibilityBuffer : register(t0);
StructuredBuffer<uint> g_InstanceIndexBuffer : register(t1);
StructuredBuffer<InstanceRange> g_InstanceRangeBuffer : register(t2);
StructuredBuffer<MeshInfo> g_MeshInfoBuffer : register(t3);

RWStructuredBuffer<uint> g_VisibleInstanceIndexBuffer : register(u0);
RWStructuredBuffer<uint> g_NumVisibleMeshesPerTypeBuffer : register(u1);
RWStructuredBuffer<DrawCommand> g_DrawVisibleInstanceCommandBuffer : register(u2);

RWStructuredBuffer<uint> g_OccludedInstanceIndexBuffer : register(u3);
RWStructuredBuffer<uint> g_NumOccludedMeshesBuffer : register(u4);
RWStructuredBuffer<InstanceRange> g_OccludedInstanceRangeBuffer : register(u5);
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

	InstanceRange instanceRange = g_InstanceRangeBuffer[groupId.x];
	for (uint index = localIndex; index < instanceRange.numInstances; index += THREAD_GROUP_SIZE)
	{
		uint instanceIndex = g_InstanceIndexBuffer[instanceRange.instanceOffset + index];
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
			MeshInfo meshInfo = g_MeshInfoBuffer[instanceRange.meshIndex];
			
			uint commandOffset;
			InterlockedAdd(g_NumVisibleMeshesPerTypeBuffer[meshInfo.meshType], 1, commandOffset);
			commandOffset += meshInfo.meshTypeOffset;
			
			g_DrawVisibleInstanceCommandBuffer[commandOffset].instanceOffset = instanceRange.instanceOffset;
			g_DrawVisibleInstanceCommandBuffer[commandOffset].materialId = meshInfo.materialId;
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
			g_OccludedInstanceRangeBuffer[meshOffset].meshIndex = instanceRange.meshIndex;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localIndex; index < g_NumVisibleInstances; index += THREAD_GROUP_SIZE)
		g_VisibleInstanceIndexBuffer[instanceRange.instanceOffset + index] = g_VisibleInstanceIndices[index];

	for (uint index = localIndex; index < g_NumOccludedInstances; index += THREAD_GROUP_SIZE)
		g_OccludedInstanceIndexBuffer[g_OccludedInstanceOffset + index] = g_OccludedInstanceIndices[index];
}
