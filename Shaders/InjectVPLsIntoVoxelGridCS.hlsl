#include "VoxelGrid.hlsl"
#include "Lighting.hlsl"

cbuffer GridConfigBuffer : register(b0)
{
	GridConfig g_GridConfig;
}

StructuredBuffer<Voxel> g_GridBuffer : register(t0);
StructuredBuffer<OmniLight> g_OmniLightsBuffer : register(t1);
StructuredBuffer<SpotLight> g_SpotLightsBuffer : register(t2);

RWStructuredBuffer<VPL> g_VPLsBuffer : register(u0);

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, NUM_THREADS_Z)]
void Main(int3 gridCell : SV_DispatchThreadID)
{
	VPL vpl;
	vpl.shRedCoeffs = float4(0.0f, 0.0f, 0.0f, 0.0f);
	vpl.shGreenCoeffs = float4(0.0f, 0.0f, 0.0f, 0.0f);
	vpl.shBlueCoeffs = float4(0.0f, 0.0f, 0.0f, 0.0f);

	int cellIndex = ComputeCellIndex(g_GridConfig, gridCell);
	
	float4 colorAndNumOccluders = g_GridBuffer[cellIndex].colorAndNumOccluders;
	if (colorAndNumOccluders.w > 0)
	{

	}

	g_VPLsBuffer[cellIndex] = vpl;
}