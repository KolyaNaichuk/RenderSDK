#include "Foundation.hlsl"
#include "OverlapTest.hlsl"

struct ShadowMapCommand
{
	uint dataOffset;
	DrawIndexedArgs args;
};

cbuffer Constants32BitBuffer : register(b1)
{
	uint g_NumPointLights;
	uint g_NumSpotLights;
}

StructuredBuffer<MeshInfo> g_MeshInfoBuffer : register(t0);
StructuredBuffer<AABB> g_MeshInstanceWorldAABBBuffer : register(t1);
Buffer<uint> g_MeshInstanceIndexBuffer : register(t2);

#if ENABLE_POINT_LIGHTS == 1
StructuredBuffer<Sphere> g_PointLightWorldBoundsBuffer : register(t3);
RWBuffer<uint> g_NumPointLightMeshInstancesBuffer : register(u0);
RWBuffer<uint> g_PointLightIndexForMeshInstanceBuffer : register(u1);
RWBuffer<uint> g_MeshInstanceIndexForPointLightBuffer : register(u2);
RWBuffer<uint> g_NumPointLightCommandsBuffer : register(u3);
RWStructuredBuffer<ShadowMapCommand> g_PointLightCommandBuffer : register(u4);
#endif // ENABLE_POINT_LIGHTS

#if ENABLE_SPOT_LIGHTS == 1
StructuredBuffer<Sphere> g_SpotLightWorldBoundsBuffer : register(t4);
RWBuffer<uint> g_NumSpotLightMeshInstancesBuffer : register(u5);
RWBuffer<uint> g_SpotLightIndexForMeshInstanceBuffer : register(u6);
RWBuffer<uint> g_MeshInstanceIndexForSpotLightBuffer : register(u7);
RWBuffer<uint> g_NumSpotLightCommandsBuffer : register(u8);
RWStructuredBuffer<ShadowMapCommand> g_SpotLightCommandBuffer : register(u9);
#endif // ENABLE_SPOT_LIGHTS

groupshared uint g_LightIndexForMeshInstance[MAX_NUM_INSTANCES_PER_MESH * MAX_NUM_LIGHTS];
groupshared uint g_MeshInstanceIndexForLight[MAX_NUM_INSTANCES_PER_MESH * MAX_NUM_LIGHTS];
groupshared uint g_NumMeshInstances;
groupshared uint g_DataOffset;

[numthreads(NUM_THREADS_PER_MESH, 1, 1)]
void Main(uint3 groupId : SV_GroupID, uint localThreadIndex : SV_GroupIndex)
{
	MeshInfo meshInfo = g_MeshInfoBuffer[groupId.x];

#if ENABLE_POINT_LIGHTS == 1
	if (localThreadIndex == 0)
		g_NumMeshInstances = 0;
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localThreadIndex; index < meshInfo.numInstances; index += NUM_THREADS_PER_MESH)
	{
		uint instanceIndex = g_MeshInstanceIndexBuffer[meshInfo.instanceOffset + index];
		AABB instanceWorldAABB = g_MeshInstanceWorldAABBBuffer[instanceIndex];

		for (uint lightIndex = 0; lightIndex < g_NumPointLights; ++lightIndex)
		{
			Sphere lightWorldBounds = g_PointLightWorldBoundsBuffer[lightIndex];
			if (TestSphereAgainstAABB(instanceWorldAABB, lightWorldBounds))
			{
				uint listIndex;
				InterlockedAdd(g_NumMeshInstances, 1, listIndex);

				g_LightIndexForMeshInstance[listIndex] = lightIndex;
				g_MeshInstanceIndexForLight[listIndex] = instanceIndex;
			}
		}
	}
	GroupMemoryBarrierWithGroupSync();

	if (localThreadIndex == 0)
	{
		if (g_NumMeshInstances > 0)
		{
			uint commandOffset;
			InterlockedAdd(g_NumPointLightCommandsBuffer[0], 1, commandOffset);
			InterlockedAdd(g_NumPointLightMeshInstancesBuffer[0], g_NumMeshInstances, g_DataOffset);

			g_PointLightCommandBuffer[commandOffset].dataOffset = g_DataOffset;
			g_PointLightCommandBuffer[commandOffset].args.indexCountPerInstance = meshInfo.indexCountPerInstance;
			g_PointLightCommandBuffer[commandOffset].args.instanceCount = g_NumMeshInstances;
			g_PointLightCommandBuffer[commandOffset].args.startIndexLocation = meshInfo.startIndexLocation;
			g_PointLightCommandBuffer[commandOffset].args.baseVertexLocation = meshInfo.baseVertexLocation;
			g_PointLightCommandBuffer[commandOffset].args.startInstanceLocation = 0;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localThreadIndex; index < g_NumMeshInstances; index += NUM_THREADS_PER_MESH)
	{
		g_PointLightIndexForMeshInstanceBuffer[g_DataOffset + index] = g_LightIndexForMeshInstance[index];
		g_MeshInstanceIndexForPointLightBuffer[g_DataOffset + index] = g_MeshInstanceIndexForLight[index];
	}
	GroupMemoryBarrierWithGroupSync();
#endif // ENABLE_POINT_LIGHTS

#if ENABLE_SPOT_LIGHTS == 1
	if (localThreadIndex == 0)
		g_NumMeshInstances = 0;
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localThreadIndex; index < meshInfo.numInstances; index += NUM_THREADS_PER_MESH)
	{
		uint instanceIndex = g_MeshInstanceIndexBuffer[meshInfo.instanceOffset + index];
		AABB instanceWorldAABB = g_MeshInstanceWorldAABBBuffer[instanceIndex];

		for (uint lightIndex = 0; lightIndex < g_NumSpotLights; ++lightIndex)
		{
			Sphere lightWorldBounds = g_SpotLightWorldBoundsBuffer[lightIndex];
			if (TestSphereAgainstAABB(instanceWorldAABB, lightWorldBounds))
			{
				uint listIndex;
				InterlockedAdd(g_NumMeshInstances, 1, listIndex);

				g_LightIndexForMeshInstance[listIndex] = lightIndex;
				g_MeshInstanceIndexForLight[listIndex] = instanceIndex;
			}
		}
	}
	GroupMemoryBarrierWithGroupSync();

	if (localThreadIndex == 0)
	{
		if (g_NumMeshInstances > 0)
		{
			uint commandOffset;
			InterlockedAdd(g_NumSpotLightCommandsBuffer[0], 1, commandOffset);
			InterlockedAdd(g_NumSpotLightMeshInstancesBuffer[0], g_NumMeshInstances, g_DataOffset);

			g_SpotLightCommandBuffer[commandOffset].dataOffset = g_DataOffset;
			g_SpotLightCommandBuffer[commandOffset].args.indexCountPerInstance = meshInfo.indexCountPerInstance;
			g_SpotLightCommandBuffer[commandOffset].args.instanceCount = g_NumMeshInstances;
			g_SpotLightCommandBuffer[commandOffset].args.startIndexLocation = meshInfo.startIndexLocation;
			g_SpotLightCommandBuffer[commandOffset].args.baseVertexLocation = meshInfo.baseVertexLocation;
			g_SpotLightCommandBuffer[commandOffset].args.startInstanceLocation = 0;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localThreadIndex; index < g_NumMeshInstances; index += NUM_THREADS_PER_MESH)
	{
		g_SpotLightIndexForMeshInstanceBuffer[g_DataOffset + index] = g_LightIndexForMeshInstance[index];
		g_MeshInstanceIndexForSpotLightBuffer[g_DataOffset + index] = g_MeshInstanceIndexForLight[index];
	}
#endif // ENABLE_SPOT_LIGHTS
}