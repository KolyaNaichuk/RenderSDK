#include "VoxelGrid.hlsl"
#include "Foundation.hlsl"

struct PSInput
{
	float4 screenSpacePos		: SV_Position;
	float4 worldSpacePos		: POSITION;
	float3 worldSpaceNormal		: NORMAL;
};

cbuffer GridConfigBuffer : register(b0)
{
	GridConfig g_GridConfig;
}

cbuffer MaterialIndexBuffer : register(b1)
{
	uint g_MaterialIndex;
}

StructuredBuffer<Material> g_MaterialBuffer : register(t0);
RasterizerOrderedStructuredBuffer<Voxel> GridBuffer : register(u0);

float3 ComulativeAverage(float3 newValue, float3 prevComAvg, float numPrevValues)
{
	return (newValue + numPrevValues * prevComAvg) / (numPrevValues + 1.0f);
}

void Main(PSInput input)
{
	int3 gridCell = ComputeGridCell(g_GridConfig, input.worldSpacePos.xyz);
	if (all(int3(-1, -1, -1) < gridCell.xyz) && all(gridCell.xyz < g_GridConfig.numCells.xyz))
	{
		int cellIndex = ComputeGridCellIndex(g_GridConfig, gridCell);
						
		GridBuffer[cellIndex].numOccluders = 1.0f;
		GridBuffer[cellIndex].diffuseColor = g_MaterialBuffer[g_MaterialIndex].diffuseColor.rgb;
		GridBuffer[cellIndex].worldSpaceNormal = normalize(input.worldSpaceNormal);
	}
}
