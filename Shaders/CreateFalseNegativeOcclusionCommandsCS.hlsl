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

groupshared uint g_NumVisibleInstances;
groupshared uint g_VisibleInstanceIndices[MAX_NUM_INSTANCES];

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void Main(uint3 groupId : SV_GroupID, uint localIndex : SV_GroupIndex)
{
	if (localIndex == 0)
	{
		g_NumVisibleInstances = 0;
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
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localIndex; index < g_NumVisibleInstances; index += THREAD_GROUP_SIZE)
		g_VisibleInstanceIndexBuffer[instanceRange.instanceOffset + index] = g_VisibleInstanceIndices[index];
}
