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

Buffer<uint> g_NumVisibleMeshesBuffer : register(t0);
Buffer<uint> g_VisibleMeshIndexBuffer : register(t1);
StructuredBuffer<MeshDesc> g_MeshDescBuffer : register(t2);
RWStructuredBuffer<DrawCommand> g_DrawCommandBuffer : register(u0);

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void Main(uint3 groupId : SV_GroupID, uint localIndex : SV_GroupIndex)
{
	uint globalIndex = groupId.x * THREAD_GROUP_SIZE + localIndex;
	uint numVisibleMeshes = g_NumVisibleMeshesBuffer[0];
	
	if (globalIndex < numVisibleMeshes)
	{
		uint meshIndex = g_VisibleMeshIndexBuffer[globalIndex];

		g_DrawCommandBuffer[globalIndex].root32BitConstant = g_MeshDescBuffer[meshIndex].materialIndex;
		g_DrawCommandBuffer[globalIndex].drawArgs.indexCountPerInstance = g_MeshDescBuffer[meshIndex].indexCount;
		g_DrawCommandBuffer[globalIndex].drawArgs.instanceCount = 1;
		g_DrawCommandBuffer[globalIndex].drawArgs.startIndexLocation = g_MeshDescBuffer[meshIndex].startIndexLocation;
		g_DrawCommandBuffer[globalIndex].drawArgs.baseVertexLocation = g_MeshDescBuffer[meshIndex].baseVertexLocation;
		g_DrawCommandBuffer[globalIndex].drawArgs.startInstanceLocation = 0;
	}
}