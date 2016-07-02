#include "Foundation.hlsl"

Buffer<uint> g_NumMeshesBuffer : register(t0);
Buffer<uint> g_MeshIndexBuffer : register(t1);
StructuredBuffer<MeshDesc> g_MeshDescBuffer : register(t2);
RWStructuredBuffer<DrawMeshCommand> g_DrawMeshCommandBuffer : register(u0);

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void Main(uint3 groupId : SV_GroupID, uint localIndex : SV_GroupIndex)
{
	uint globalIndex = groupId.x * THREAD_GROUP_SIZE + localIndex;
	if (globalIndex < g_NumMeshesBuffer[0])
	{
		uint meshIndex = g_MeshIndexBuffer[globalIndex];

		g_DrawMeshCommandBuffer[globalIndex].root32BitConstant = g_MeshDescBuffer[meshIndex].materialIndex;
		g_DrawMeshCommandBuffer[globalIndex].drawArgs.indexCountPerInstance = g_MeshDescBuffer[meshIndex].indexCount;
		g_DrawMeshCommandBuffer[globalIndex].drawArgs.instanceCount = 1;
		g_DrawMeshCommandBuffer[globalIndex].drawArgs.startIndexLocation = g_MeshDescBuffer[meshIndex].startIndexLocation;
		g_DrawMeshCommandBuffer[globalIndex].drawArgs.baseVertexLocation = g_MeshDescBuffer[meshIndex].baseVertexLocation;
		g_DrawMeshCommandBuffer[globalIndex].drawArgs.startInstanceLocation = 0;
	}
}