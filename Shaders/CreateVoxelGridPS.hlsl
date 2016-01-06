#include "VoxelGrid.hlsl"

struct PSInput
{
	float4 screenSpacePos		: SV_Position;
	float4 worldSpacePos		: POSITION;
	float3 worldSpaceNormal		: NORMAL;
	
#ifdef HAS_TEXCOORD
	float2 texCoord				: TEXCOORD;
#else // HAS_COLOR
	float4 color				: COLOR;
#endif // HAS_TEXCOORD
};

cbuffer GridConfigBuffer : register(b0)
{
	GridConfig g_GridConfig;
}

#ifdef HAS_TEXCOORD
Texture2D ColorTexture : register(t0);
SamplerState LinearSampler : register(s0);
#endif // HAS_TEXCOORD

RasterizerOrderedStructuredBuffer<Voxel> GridBuffer : register(u0);

float4 ComulativeAverage(float3 newValue, float4 prevComAvgAndNum)
{
	float3 comAvg = (newValue.xyz + prevComAvgAndNum.w * prevComAvgAndNum.xyz) / (prevComAvgAndNum.w + 1.0f);
	return float4(comAvg.xyz, prevComAvgAndNum.w + 1.0f);
}

void Main(PSInput input)
{
	int3 gridCell = ComputeGridCell(g_GridConfig, input.worldSpacePos.xyz);
	if (all(float3(-1.0f, -1.0f, -1.0f) < gridCell.xyz) && all(gridCell.xyz < g_GridConfig.numCells.xyz))
	{
		int cellIndex = ComputeCellIndex(g_GridConfig, gridCell);

#ifdef HAS_TEXCOORD
		float3 color = ColorTexture.Sample(LinearSampler, input.texCoord).rgb;
#else // HAS_COLOR
		float3 color = input.color.rgb;
#endif // HAS_TEXCOORD
		
		GridBuffer[cellIndex].colorAndNumOccluders = ComulativeAverage(color, GridBuffer[cellIndex].colorAndNumOccluders);
	}
}