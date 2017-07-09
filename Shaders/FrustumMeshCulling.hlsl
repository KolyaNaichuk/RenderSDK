struct InstanceRange
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
	uint materialId;
	DrawIndexedArgs args;
};

cbuffer FrustumCullingDataBuffer : register(b0)
{
	FrustumCullingData g_CullingData;
}

StructuredBuffer<AABB> g_InstanceAABBBuffer : register(t0);
StructuredBuffer<InstanceRange> g_InstanceRangeBuffer : register(t1);

RWStructuredBuffer<uint> g_NumVisibleMeshesBuffer : register(u0);
RWStructuredBuffer<InstanceRange> g_VisibleInstanceRangeBuffer : register(u1);
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
 
	InstanceRange instanceRange = g_InstanceRangeBuffer[groupId.x];
	for (uint index = localIndex; index < instanceRange.numInstances; index += THREAD_GROUP_SIZE)
	{
		uint instanceIndex = instanceRange.instanceOffset + index;
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
			g_VisibleInstanceRangeBuffer[meshOffset].meshIndex = instanceRange.meshIndex;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localIndex; index < g_NumVisibleInstances; index += THREAD_GROUP_SIZE)
		g_VisibleInstanceIndexBuffer[g_VisibleInstanceOffset + index] = g_VisibleInstanceIndices[index];
}