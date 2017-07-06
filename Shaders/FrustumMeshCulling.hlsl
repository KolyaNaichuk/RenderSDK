struct MeshInfo
{
	uint instanceOffset;
	uint numInstances;
	uint meshIndex;
};

struct FrustumCullingData
{
	float4 frustumPlanes[6];
	float4 notUsed[10];
};

StructuredBuffer<AABB> g_InstanceAABBBuffer : register(t0);
StructuredBuffer<MeshInfo> g_MeshInfoBuffer : register(t1);

RWStructuredBuffer<uint> g_NumVisibleMeshesBuffer : register(u0);
RWStructuredBuffer<DrawMeshCommand> g_DrawOOBCommandBuffer : register(u1);
RWStructuredBuffer<MeshInfo> g_VisibleMeshInfoBuffer : register(u2);
RWStructuredBuffer<uint> g_NumVisibleInstancesBuffer : register(u3);
RWStructuredBuffer<uint> g_VisibleInstanceIndexBuffer : register(u4);

groupshared uint g_NumVisibleInstances;
groupshared uint g_VisibleInstanceIndices[MAX_NUM_INSTANCES];
groupshared uint g_VisibleInstanceOffset;

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void Main(uint3 groupId : SV_GroupID, uint localIndex : SV_GroupIndex)
{
	if (localIndex == 0)
	{
		g_NumVisibleInstances = 0;
		g_VisibleInstanceOffset = 0;
	}
	GroupMemoryBarrierWithGroupSync();
 
	MeshInfo meshInfo = g_MeshInfoBuffer[groupId.x];
	for (uint index = localIndex; index < meshInfo.numInstances; index += THREAD_GROUP_SIZE)
	{
		uint globalIndex = meshInfo.instanceOffset + index;
		if (TestAABBAgainstFrustum(g_CullingData.frustumPlanes, g_InstanceAABBBuffer[globalIndex]))
		{
			uint listIndex;
			InterlockedAdd(g_NumVisibleInstances, 1, listIndex);
			g_VisibleInstanceIndices[listIndex] = globalIndex;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	if (localIndex == 0)
	{
		if (g_NumVisibleInstances > 0)
		{
			uint instanceOffset;
			InterlockedAdd(g_NumVisibleInstancesBuffer[0], g_NumVisibleInstances, instanceOffset);
			g_VisibleInstanceOffset = instanceOffset;

			uint meshOffset;
			InterlockedAdd(g_NumVisibleMeshesBuffer[0], 1, meshOffset);

			g_VisibleMeshInfoBuffer[meshOffset].instanceOffset = instanceOffset;
			g_VisibleMeshInfoBuffer[meshOffset].numInstances = g_NumVisibleInstances;
			g_VisibleMeshInfoBuffer[meshOffset].meshIndex = meshInfo.meshIndex;

			g_DrawOOBCommandBuffer[meshOffset].root32BitConstant = instanceOffset;
			g_DrawOOBCommandBuffer[meshOffset].drawArgs.indexCountPerInstance = 8;
			g_DrawOOBCommandBuffer[meshOffset].drawArgs.instanceCount = g_NumVisibleInstances;
			g_DrawOOBCommandBuffer[meshOffset].drawArgs.startIndexLocation = 0;
			g_DrawOOBCommandBuffer[meshOffset].drawArgs.baseVertexLocation = 0;
			g_DrawOOBCommandBuffer[meshOffset].drawArgs.startInstanceLocation = 0;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localIndex; index < g_NumVisibleInstances; index += THREAD_GROUP_SIZE)
		g_VisibleInstanceIndexBuffer[g_VisibleInstanceOffset + index] = g_VisibleInstanceIndices[index];
}