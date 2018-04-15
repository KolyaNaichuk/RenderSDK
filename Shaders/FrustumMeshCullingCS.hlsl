#include "Foundation.hlsl"
#include "OverlapTest.hlsl"

cbuffer AppDataBuffer : register(b0)
{
	AppData g_AppData;
}

StructuredBuffer<MeshInfo> g_MeshInfoBuffer : register(t0);
StructuredBuffer<AABB> g_InstanceWorldAABBBuffer : register(t1);

RWBuffer<uint> g_NumVisibleMeshesBuffer : register(u0);
RWBuffer<uint> g_NumVisibleInstancesBuffer : register(u1);
RWStructuredBuffer<MeshInfo> g_VisibleMeshInfoBuffer : register(u2);
RWBuffer<uint> g_VisibleInstanceIndexBuffer : register(u3);

groupshared uint g_NumVisibleInstancesPerMesh;
groupshared uint g_VisibleInstanceIndicesPerMesh[MAX_NUM_INSTANCES_PER_MESH];
groupshared uint g_VisibleInstanceOffsetPerMesh;

[numthreads(NUM_THREADS_PER_MESH, 1, 1)]
void Main(uint3 groupId : SV_GroupID, uint localIndex : SV_GroupIndex)
{
	if (localIndex == 0)
	{
		g_NumVisibleInstancesPerMesh = 0;
		g_VisibleInstanceOffsetPerMesh = 0;
	}
	GroupMemoryBarrierWithGroupSync();
 
	MeshInfo meshInfo = g_MeshInfoBuffer[groupId.x];
	for (uint index = localIndex; index < meshInfo.numInstances; index += NUM_THREADS_PER_MESH)
	{
		uint instanceIndex = meshInfo.instanceOffset + index;
		if (TestAABBAgainstFrustum(g_AppData.cameraWorldFrustumPlanes, g_InstanceWorldAABBBuffer[instanceIndex]))
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
			uint instanceOffset;
			InterlockedAdd(g_NumVisibleInstancesBuffer[0], g_NumVisibleInstancesPerMesh, instanceOffset);
			g_VisibleInstanceOffsetPerMesh = instanceOffset;

			uint meshOffset;
			InterlockedAdd(g_NumVisibleMeshesBuffer[0], 1, meshOffset);
			g_VisibleMeshInfoBuffer[meshOffset].numInstances = g_NumVisibleInstancesPerMesh;
			g_VisibleMeshInfoBuffer[meshOffset].instanceOffset = instanceOffset;
			g_VisibleMeshInfoBuffer[meshOffset].meshType = meshInfo.meshType;
			g_VisibleMeshInfoBuffer[meshOffset].meshTypeOffset = meshInfo.meshTypeOffset;
			g_VisibleMeshInfoBuffer[meshOffset].materialID = meshInfo.materialID;
			g_VisibleMeshInfoBuffer[meshOffset].indexCountPerInstance = meshInfo.indexCountPerInstance;
			g_VisibleMeshInfoBuffer[meshOffset].startIndexLocation = meshInfo.startIndexLocation;
			g_VisibleMeshInfoBuffer[meshOffset].baseVertexLocation = meshInfo.baseVertexLocation;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localIndex; index < g_NumVisibleInstancesPerMesh; index += NUM_THREADS_PER_MESH)
		g_VisibleInstanceIndexBuffer[g_VisibleInstanceOffsetPerMesh + index] = g_VisibleInstanceIndicesPerMesh[index];
}