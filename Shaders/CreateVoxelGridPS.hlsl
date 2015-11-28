#include "VoxelGrid.hlsl"

struct PSInput
{
	float4 screenSpacePos		: SV_Position;
	float4 worldSpacePos		: POSITION;
	float3 worldSpaceNormal		: NORMAL;
	float2 texCoord				: TEXCOORD;
};

cbuffer GridConfigBuffer : register(b0)
{
	GridConfig g_GridConfig;
}

Texture2D ColorTexture : register(t0);
SamplerState TextureSampler : register(s0);
RasterizerOrderedStructuredBuffer<Voxel> GridBuffer : register(u0);

float4 ComulativeAverage(float3 newValue, float4 prevComAvgAndNum)
{
	float3 comAvg = (newValue.xyz + prevComAvgAndNum.w * prevComAvgAndNum.xyz) / (prevComAvgAndNum.w + 1.0f);
	return float4(comAvg.xyz, prevComAvgAndNum.w + 1.0f);
}

void Main(PSInput input)
{
	int3 gridCell = ComputeGridCell(g_GridConfig, input.worldSpacePos.xyz);
	if (all(float3(-1.0f, -1.0f, -1.0f) < gridCell.xyz) && all(gridCell.xyz < GridNumCells.xyz))
	{
		int cellIndex = ComputeCellIndex(g_GridConfig, gridCell);
		float3 color = ColorTexture.Sample(TextureSampler, input.texCoord).rgb;

		GridBuffer[cellIndex].colorAndNumOccluders = ComulativeAverage(color, GridBuffer[cellIndex].colorAndNumOccluders);
	}
}