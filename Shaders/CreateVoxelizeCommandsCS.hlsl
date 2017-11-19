#include "Foundation.hlsl"

#ifdef SET_ARGUMENTS
Buffer<uint> g_NumMeshesBuffer : register(t0);
RWStructuredBuffer<DispatchArgs> g_ArgumentBuffer : register(u0);

[numthreads(1, 1, 1)]
void Main()
{
	g_ArgumentBuffer[0].threadGroupCountX = uint(ceil(float(g_NumMeshesBuffer[0]) / float(NUM_THREADS_PER_GROUP)));
	g_ArgumentBuffer[0].threadGroupCountY = 1;
	g_ArgumentBuffer[0].threadGroupCountZ = 1;
}
#endif // SET_ARGUMENTS

#ifdef CREATE_COMMANDS
RWBuffer<uint> g_NumCommandsPerMeshTypeBuffer : register(u0);
RWStructuredBuffer<DrawCommand> g_VoxelizeCommandBuffer : register(u1);
Buffer<uint> g_NumMeshesBuffer : register(t0);
StructuredBuffer<MeshInfo> g_MeshInfoBuffer : register(t1);

groupshared uint g_NumCommandsPerMeshType[NUM_MESH_TYPES];
groupshared uint g_GlobalOffset[NUM_MESH_TYPES];

[numthreads(NUM_THREADS_PER_GROUP, 1, 1)]
void Main(uint3 globalThreadId : SV_DispatchThreadID, uint localThreadIndex : SV_GroupIndex)
{
	for (uint index = localThreadIndex; index < NUM_MESH_TYPES; index += NUM_THREADS_PER_GROUP)
	{
		g_NumCommandsPerMeshType[index] = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	const uint meshIndex = globalThreadId.x;
	const uint numMeshes = g_NumMeshesBuffer[0];

	uint localOffset;
	if (meshIndex < numMeshes)
	{
		MeshInfo meshInfo = g_MeshInfoBuffer[meshIndex];
		InterlockedAdd(g_NumCommandsPerMeshType[meshInfo.meshType], 1, localOffset);
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localThreadIndex; index < NUM_MESH_TYPES; index += NUM_THREADS_PER_GROUP)
	{
		uint globalOffset;
		InterlockedAdd(g_NumCommandsPerMeshTypeBuffer[index], g_NumCommandsPerMeshType[index], globalOffset);
		g_GlobalOffset[index] = globalOffset;
	}
	GroupMemoryBarrierWithGroupSync();

	if (meshIndex < numMeshes)
	{
		MeshInfo meshInfo = g_MeshInfoBuffer[meshIndex];
		uint commandOffset = meshInfo.meshTypeOffset + g_GlobalOffset[meshInfo.meshType] + localOffset;

		g_VoxelizeCommandBuffer[commandOffset].instanceOffset = meshInfo.instanceOffset;
		g_VoxelizeCommandBuffer[commandOffset].materialID = meshInfo.materialID;
		g_VoxelizeCommandBuffer[commandOffset].args.indexCountPerInstance = meshInfo.indexCountPerInstance;
		g_VoxelizeCommandBuffer[commandOffset].args.instanceCount = meshInfo.numInstances;
		g_VoxelizeCommandBuffer[commandOffset].args.startIndexLocation = meshInfo.startIndexLocation;
		g_VoxelizeCommandBuffer[commandOffset].args.baseVertexLocation = meshInfo.baseVertexLocation;
		g_VoxelizeCommandBuffer[commandOffset].args.startInstanceLocation = 0;
	}
}
#endif // CREATE_COMMANDS
