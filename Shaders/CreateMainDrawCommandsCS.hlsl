#include "Foundation.hlsl"

struct DrawCommand
{
	uint instanceOffset;
	uint materialID;
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

groupshared uint g_NumVisibleInstancesPerMesh;
groupshared uint g_VisibleInstanceIndicesPerMesh[MAX_NUM_INSTANCES_PER_MESH];

groupshared uint g_NumOccludedInstancesPerMesh;
groupshared uint g_OccludedInstanceIndicesPerMesh[MAX_NUM_INSTANCES_PER_MESH];
groupshared uint g_OccludedInstanceOffsetPerMesh;

[numthreads(NUM_THREADS_PER_MESH, 1, 1)]
void Main(uint3 groupId : SV_GroupID, uint localIndex : SV_GroupIndex)
{
	if (localIndex == 0)
	{
		g_NumVisibleInstancesPerMesh = 0;
		g_NumOccludedInstancesPerMesh = 0;
		g_OccludedInstanceOffsetPerMesh = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	MeshInfo meshInfo = g_MeshInfoBuffer[groupId.x];
	for (uint index = localIndex; index < meshInfo.numInstances; index += NUM_THREADS_PER_MESH)
	{
		uint instanceIndex = g_InstanceIndexBuffer[meshInfo.instanceOffset + index];
		if (g_VisibilityBuffer[instanceIndex] > 0)
		{
			uint listIndex;
			InterlockedAdd(g_NumVisibleInstancesPerMesh, 1, listIndex);
			g_VisibleInstanceIndicesPerMesh[listIndex] = instanceIndex;
		}
		else
		{
			uint listIndex;
			InterlockedAdd(g_NumOccludedInstancesPerMesh, 1, listIndex);
			g_OccludedInstanceIndicesPerMesh[listIndex] = instanceIndex;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	if (localIndex == 0)
	{
		if (g_NumVisibleInstancesPerMesh > 0)
		{
			uint commandOffset;
			InterlockedAdd(g_NumVisibleMeshesPerTypeBuffer[meshInfo.meshType], 1, commandOffset);
			commandOffset += meshInfo.meshTypeOffset;
			
			g_DrawCommandBuffer[commandOffset].instanceOffset = meshInfo.instanceOffset;
			g_DrawCommandBuffer[commandOffset].materialID = meshInfo.materialID;
			g_DrawCommandBuffer[commandOffset].args.indexCountPerInstance = meshInfo.indexCountPerInstance;
			g_DrawCommandBuffer[commandOffset].args.instanceCount = g_NumVisibleInstancesPerMesh;
			g_DrawCommandBuffer[commandOffset].args.startIndexLocation = meshInfo.startIndexLocation;
			g_DrawCommandBuffer[commandOffset].args.baseVertexLocation = meshInfo.baseVertexLocation;
			g_DrawCommandBuffer[commandOffset].args.startInstanceLocation = 0;
		}
		if (g_NumOccludedInstancesPerMesh > 0)
		{
			uint instanceOffset;
			InterlockedAdd(g_NumOccludedInstancesBuffer[0], g_NumOccludedInstancesPerMesh, instanceOffset);
			g_OccludedInstanceOffsetPerMesh = instanceOffset;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localIndex; index < g_NumVisibleInstancesPerMesh; index += NUM_THREADS_PER_MESH)
		g_VisibleInstanceIndexBuffer[meshInfo.instanceOffset + index] = g_VisibleInstanceIndicesPerMesh[index];

	for (uint index = localIndex; index < g_NumOccludedInstancesPerMesh; index += NUM_THREADS_PER_MESH)
		g_OccludedInstanceIndexBuffer[g_OccludedInstanceOffsetPerMesh + index] = g_OccludedInstanceIndicesPerMesh[index];
}
