#include "Foundation.hlsl"

Buffer<uint> g_VisibilityBuffer : register(t0);
Buffer<uint> g_InstanceIndexBuffer : register(t1);
StructuredBuffer<MeshInfo> g_MeshInfoBuffer : register(t2);

RWBuffer<uint> g_VisibleInstanceIndexBuffer : register(u0);
RWBuffer<uint> g_NumVisibleMeshesPerTypeBuffer : register(u1);
RWStructuredBuffer<DrawCommand> g_DrawCommandBuffer : register(u2);

groupshared uint g_NumVisibleInstancesPerMesh;
groupshared uint g_VisibleInstanceIndicesPerMesh[MAX_NUM_INSTANCES_PER_MESH];

[numthreads(NUM_THREADS_PER_MESH, 1, 1)]
void Main(uint3 groupId : SV_GroupID, uint localIndex : SV_GroupIndex)
{
	if (localIndex == 0)
	{
		g_NumVisibleInstancesPerMesh = 0;
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
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localIndex; index < g_NumVisibleInstancesPerMesh; index += NUM_THREADS_PER_MESH)
		g_VisibleInstanceIndexBuffer[meshInfo.instanceOffset + index] = g_VisibleInstanceIndicesPerMesh[index];
}
