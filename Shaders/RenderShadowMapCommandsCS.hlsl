#include "Foundation.hlsl"
#include "OverlapTest.hlsl"

Buffer<uint> g_MeshIndexBuffer : register(t0);
StructuredBuffer<AABB> g_MeshBoundsBuffer : register(t1);
StructuredBuffer<MeshDesc> g_MeshDescBuffer : register(t2);

#if ENABLE_POINT_LIGHTS == 1
StructuredBuffer<Sphere> g_PointLightBoundsBuffer : register(t3);
Buffer<uint> g_NumPointLightsBuffer : register(t4);
Buffer<uint> g_PointLightIndexBuffer : register(t5);

RWBuffer<uint> g_ShadowCastingPointLightIndexBuffer : register(u0);
RWBuffer<uint> g_NumShadowCastingPointLightsBuffer : register(u1);

RWStructuredBuffer<DrawMeshCommand> g_DrawPointLightShadowCasterCommandBuffer : register(u2);
RWBuffer<uint> g_NumDrawPointLightShadowCastersBuffer : register(u3);

groupshared uint g_PointLightIndicesPerShadowCaster[MAX_NUM_POINT_LIGHTS_PER_SHADOW_CASTER];
groupshared uint g_NumPointLightsPerShadowCaster;
groupshared uint g_ShadowCastingPointLightOffset;
#endif

#if ENABLE_SPOT_LIGHTS == 1
StructuredBuffer<Sphere> g_SpotLightBoundsBuffer : register(t6);
Buffer<uint> g_NumSpotLightsBuffer : register(t7);
Buffer<uint> g_SpotLightIndexBuffer : register(t8);

RWBuffer<uint> g_ShadowCastingSpotLightIndexBuffer : register(u4);
RWBuffer<uint> g_NumShadowCastingSpotLightsBuffer : register(u5);

RWStructuredBuffer<DrawMeshCommand> g_DrawSpotLightShadowCasterCommandBuffer : register(u6);
RWBuffer<uint> g_NumDrawSpotLightShadowCastersBuffer : register(u7);

groupshared uint g_SpotLightIndicesPerShadowCaster[MAX_NUM_SPOT_LIGHTS_PER_SHADOW_CASTER];
groupshared uint g_NumSpotLightsPerShadowCaster;
groupshared uint g_ShadowCastingSpotLightOffset;
#endif

#if ENABLE_POINT_LIGHTS == 1
void FindPointLightsPerShadowCaster(uint localThreadIndex, AABB meshBounds)
{
	for (uint index = localThreadIndex; index < g_NumPointLightsBuffer[0]; index += THREAD_GROUP_SIZE)
	{
		uint lightIndex = g_PointLightIndexBuffer[index];
		if (TestSphereAgainstAABB(meshBounds, g_PointLightBoundsBuffer[lightIndex]))
		{
			uint listIndex;
			InterlockedAdd(g_NumPointLightsPerShadowCaster, 1, listIndex);
			g_PointLightIndicesPerShadowCaster[listIndex] = lightIndex;
		}
	}
}
#endif

#if ENABLE_SPOT_LIGHTS == 1
void FindSpotLightsPerShadowCaster(uint localThreadIndex, AABB meshBounds)
{
	for (uint index = localThreadIndex; index < g_NumSpotLightsBuffer[0]; index += THREAD_GROUP_SIZE)
	{
		uint lightIndex = g_SpotLightIndexBuffer[index];
		if (TestSphereAgainstAABB(meshBounds, g_SpotLightBoundsBuffer[lightIndex]))
		{
			uint listIndex;
			InterlockedAdd(g_NumSpotLightsPerShadowCaster, 1, listIndex);
			g_SpotLightIndicesPerShadowCaster[listIndex] = lightIndex;
		}
	}
}
#endif

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void Main(uint3 groupId : SV_GroupID, uint localThreadIndex : SV_GroupIndex)
{
	if (localThreadIndex == 0)
	{
#if ENABLE_POINT_LIGHTS == 1
		g_NumPointLightsPerShadowCaster = 0;
		g_ShadowCastingPointLightOffset = 0;
#endif

#if ENABLE_SPOT_LIGHTS == 1
		g_NumSpotLightsPerShadowCaster = 0;
		g_ShadowCastingSpotLightOffset = 0;
#endif
	}
	GroupMemoryBarrierWithGroupSync();

	uint globalThreadIndex = groupId.x;
	uint meshIndex = g_MeshIndexBuffer[globalThreadIndex];
	AABB meshBounds = g_MeshBoundsBuffer[meshIndex];

#if ENABLE_POINT_LIGHTS == 1
	FindPointLightsPerShadowCaster(localThreadIndex, meshBounds);
	GroupMemoryBarrierWithGroupSync();
#endif

#if ENABLE_SPOT_LIGHTS == 1
	FindSpotLightsPerShadowCaster(localThreadIndex, meshBounds);
	GroupMemoryBarrierWithGroupSync();
#endif

	if (localThreadIndex == 0)
	{
#if ENABLE_POINT_LIGHTS == 1
		if (g_NumPointLightsPerShadowCaster > 0)
		{
			uint pointLightOffset;
			InterlockedAdd(g_NumShadowCastingPointLightsBuffer[0], g_NumPointLightsPerShadowCaster, pointLightOffset);
			g_ShadowCastingPointLightOffset = pointLightOffset;

			uint commandOffset;
			InterlockedAdd(g_NumDrawPointLightShadowCastersBuffer[0], 1, commandOffset);

			g_DrawPointLightShadowCasterCommandBuffer[commandOffset].root32BitConstant = pointLightOffset;
			g_DrawPointLightShadowCasterCommandBuffer[commandOffset].drawArgs.indexCountPerInstance = g_MeshDescBuffer[meshIndex].indexCount;
			g_DrawPointLightShadowCasterCommandBuffer[commandOffset].drawArgs.instanceCount = g_NumPointLightsPerShadowCaster;
			g_DrawPointLightShadowCasterCommandBuffer[commandOffset].drawArgs.startIndexLocation = g_MeshDescBuffer[meshIndex].startIndexLocation;
			g_DrawPointLightShadowCasterCommandBuffer[commandOffset].drawArgs.baseVertexLocation = g_MeshDescBuffer[meshIndex].baseVertexLocation;
			g_DrawPointLightShadowCasterCommandBuffer[commandOffset].drawArgs.startInstanceLocation = 0;
		}
#endif

#if ENABLE_SPOT_LIGHTS == 1
		if (g_NumSpotLightsPerShadowCaster > 0)
		{
			uint spotLightOffset;
			InterlockedAdd(g_NumShadowCastingSpotLightsBuffer[0], g_NumSpotLightsPerShadowCaster, spotLightOffset);
			g_ShadowCastingSpotLightOffset = spotLightOffset;

			uint commandOffset;
			InterlockedAdd(g_NumDrawSpotLightShadowCastersBuffer[0], 1, commandOffset);

			g_DrawSpotLightShadowCasterCommandBuffer[commandOffset].root32BitConstant = spotLightOffset;
			g_DrawSpotLightShadowCasterCommandBuffer[commandOffset].drawArgs.indexCountPerInstance = g_MeshDescBuffer[meshIndex].indexCount;
			g_DrawSpotLightShadowCasterCommandBuffer[commandOffset].drawArgs.instanceCount = g_NumSpotLightsPerShadowCaster;
			g_DrawSpotLightShadowCasterCommandBuffer[commandOffset].drawArgs.startIndexLocation = g_MeshDescBuffer[meshIndex].startIndexLocation;
			g_DrawSpotLightShadowCasterCommandBuffer[commandOffset].drawArgs.baseVertexLocation = g_MeshDescBuffer[meshIndex].baseVertexLocation;
			g_DrawSpotLightShadowCasterCommandBuffer[commandOffset].drawArgs.startInstanceLocation = 0;
		}
#endif
	}
	GroupMemoryBarrierWithGroupSync();

#if ENABLE_POINT_LIGHTS == 1
	for (uint index = localThreadIndex; index < g_NumPointLightsPerShadowCaster; index += THREAD_GROUP_SIZE)
		g_ShadowCastingPointLightIndexBuffer[g_ShadowCastingPointLightOffset + index] = g_PointLightIndicesPerShadowCaster[index];
#endif

#if ENABLE_SPOT_LIGHTS == 1
	for (uint index = localThreadIndex; index < g_NumSpotLightsPerShadowCaster; index += THREAD_GROUP_SIZE)
		g_ShadowCastingSpotLightIndexBuffer[g_ShadowCastingSpotLightOffset + index] = g_SpotLightIndicesPerShadowCaster[index];
#endif
}