#include "OverlapTest.hlsl"

#define OBJECT_BOUNDS_TYPE_AABB		1
#define OBJECT_BOUNDS_TYPE_SPHERE	2

struct CullingData
{
	float4 viewFrustumPlanes[6];
	uint numObjects;
	uint3 notUsed1;
	float4 notUsed2[9];
};

cbuffer CullingDataBuffer : register(b0)
{
	CullingData g_CullingData;
};

#if OBJECT_BOUNDS_TYPE == OBJECT_BOUNDS_TYPE_AABB
StructuredBuffer<AABB> g_ObjectBoundsBuffer : register(t0);
#elif OBJECT_BOUNDS_TYPE == OBJECT_BOUNDS_TYPE_SPHERE
StructuredBuffer<Sphere> g_ObjectBoundsBuffer : register(t0);
#else
#error Undefined object bounds type
#endif

RWBuffer<uint> g_NumVisibleObjectsBuffer : register(u0);
RWBuffer<uint> g_VisibleObjectIndexBuffer : register(u1);

groupshared uint g_NumVisibleObjects;
groupshared uint g_VisibleObjectIndices[THREAD_GROUP_SIZE];
groupshared uint g_ObjectIndicesOffset;

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void Main(uint3 groupId : SV_GroupID, uint localIndex : SV_GroupIndex)
{
	if (localIndex == 0)
	{
		g_NumVisibleObjects = 0;
		g_ObjectIndicesOffset = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	uint globalIndex = groupId.x * THREAD_GROUP_SIZE + localIndex;
	if (globalIndex < g_CullingData.numObjects)
	{
#if OBJECT_BOUNDS_TYPE == OBJECT_BOUNDS_TYPE_AABB
		if (TestAABBAgainstFrustum(g_CullingData.viewFrustumPlanes, g_ObjectBoundsBuffer[globalIndex]))
#elif OBJECT_BOUNDS_TYPE == OBJECT_BOUNDS_TYPE_SPHERE
		if (TestSphereAgainstFrustum(g_CullingData.viewFrustumPlanes, g_ObjectBoundsBuffer[globalIndex]))
#endif
		{
			uint listIndex;
			InterlockedAdd(g_NumVisibleObjects, 1, listIndex);			
			g_VisibleObjectIndices[listIndex] = globalIndex;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	if (localIndex == 0)
	{
		uint objectIndicesOffset;
		InterlockedAdd(g_NumVisibleObjectsBuffer[0], g_NumVisibleObjects, objectIndicesOffset);
		g_ObjectIndicesOffset = objectIndicesOffset;
	}
	GroupMemoryBarrierWithGroupSync();

	if (localIndex < g_NumVisibleObjects)
		g_VisibleObjectIndexBuffer[g_ObjectIndicesOffset + localIndex] = g_VisibleObjectIndices[localIndex];
}
