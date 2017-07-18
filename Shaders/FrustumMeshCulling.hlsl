struct MeshInstanceRange
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

struct DrawCommand
{
	uint instanceOffset;
	uint materialIndex;
	DrawIndexedArgs args;
};

cbuffer FrustumCullingDataBuffer : register(b0)
{
	FrustumCullingData g_CullingData;
}

StructuredBuffer<AABB> g_InstanceAABBBuffer : register(t0);
StructuredBuffer<MeshInstanceRange> g_MeshInstanceRangeBuffer : register(t1);

RWStructuredBuffer<uint> g_NumVisibleMeshesBuffer : register(u0);
RWStructuredBuffer<MeshInstanceRange> g_VisibleInstanceRangeBuffer : register(u1);
RWStructuredBuffer<DrawCommand> g_DrawVisibleInstanceCommandBuffer : register(u2);
RWStructuredBuffer<uint> g_VisibleInstanceIndexBuffer : register(u3);

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
 
	MeshInstanceRange meshInstanceRange = g_MeshInstanceRangeBuffer[groupId.x];
	for (uint index = localIndex; index < meshInstanceRange.numInstances; index += THREAD_GROUP_SIZE)
	{
		uint instanceIndex = meshInstanceRange.instanceOffset + index;
		if (TestAABBAgainstFrustum(g_CullingData.frustumPlanes, g_InstanceAABBBuffer[instanceIndex]))
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
			uint instanceOffset;
			InterlockedAdd(g_DrawVisibleInstanceCommandBuffer[0].args.instanceCount, g_NumVisibleInstances, instanceOffset);
			g_VisibleInstanceOffset = instanceOffset;

			uint meshOffset;
			InterlockedAdd(g_NumVisibleMeshesBuffer[0], 1, meshOffset);
			g_VisibleInstanceRangeBuffer[meshOffset].instanceOffset = instanceOffset;
			g_VisibleInstanceRangeBuffer[meshOffset].numInstances = g_NumVisibleInstances;
			g_VisibleInstanceRangeBuffer[meshOffset].meshIndex = meshInstanceRange.meshIndex;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localIndex; index < g_NumVisibleInstances; index += THREAD_GROUP_SIZE)
		g_VisibleInstanceIndexBuffer[g_VisibleInstanceOffset + index] = g_VisibleInstanceIndices[index];
}