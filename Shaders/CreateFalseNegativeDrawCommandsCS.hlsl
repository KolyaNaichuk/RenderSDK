#include "Foundation.hlsl"

struct DrawCommand
{
	uint instanceOffset;
	uint materialID;
	DrawIndexedArgs args;
};

StructuredBuffer<uint> g_VisibilityBuffer : register(t0);
StructuredBuffer<uint> g_InstanceIndexBuffer : register(t1);
StructuredBuffer<MeshInfo> g_MeshInfoBuffer : register(t2);

RWStructuredBuffer<uint> g_VisibleInstanceIndexBuffer : register(u0);
RWStructuredBuffer<uint> g_NumVisibleMeshesPerTypeBuffer : register(u1);
RWStructuredBuffer<DrawCommand> g_DrawCommandBuffer : register(u2);

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
	}
	GroupMemoryBarrierWithGroupSync();

	if (localIndex == 0)
	{
		if (g_NumVisibleInstances > 0)
		{
			uint commandOffset;
			InterlockedAdd(g_NumVisibleMeshesPerTypeBuffer[meshInfo.meshType], 1, commandOffset);
			commandOffset += meshInfo.meshTypeOffset;

			g_DrawVisibleInstanceCommandBuffer[commandOffset].instanceOffset = meshInfo.instanceOffset;
			g_DrawVisibleInstanceCommandBuffer[commandOffset].materialID = meshInfo.materialID;
			g_DrawVisibleInstanceCommandBuffer[commandOffset].args.indexCountPerInstance = meshInfo.indexCountPerInstance;
			g_DrawVisibleInstanceCommandBuffer[commandOffset].args.instanceCount = g_NumVisibleInstances;
			g_DrawVisibleInstanceCommandBuffer[commandOffset].args.startIndexLocation = meshInfo.startIndexLocation;
			g_DrawVisibleInstanceCommandBuffer[commandOffset].args.baseVertexLocation = meshInfo.baseVertexLocation;
			g_DrawVisibleInstanceCommandBuffer[commandOffset].args.startInstanceLocation = 0;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localIndex; index < g_NumVisibleInstances; index += THREAD_GROUP_SIZE)
		g_VisibleInstanceIndexBuffer[meshInfo.instanceOffset + index] = g_VisibleInstanceIndices[index];
}
