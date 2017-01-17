#include "VoxelGrid.hlsl"

cbuffer GridConfigBuffer : register(b0)
{
	GridConfig g_GridConfig;
}

RWStructuredBuffer<Voxel> GridBuffer : register(u0);

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, NUM_THREADS_Z)]
void Main(int3 gridCell : SV_DispatchThreadID)
{
	int cellIndex = ComputeGridCellIndex(g_GridConfig, gridCell);
	GridBuffer[cellIndex].colorAndNumOccluders = float4(0.0f, 0.0f, 0.0f, 0.0f);
}