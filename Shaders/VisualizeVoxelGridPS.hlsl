#include "Reconstruction.hlsl"
#include "VoxelGrid.hlsl"

struct PSInput
{
	float4 screenSpacePos	: SV_Position;
	float2 texCoord			: TEXCOORD0;
};

cbuffer GridConfigBuffer : register(b0)
{
	GridConfig g_GridConfig;
}

cbuffer CameraTransformBuffer : register(b1)
{
	CameraTransform g_Transform;
}

Texture2D DepthTexture : register(t0);
StructuredBuffer<Voxel> GridBuffer : register(t1);

float4 Main(PSInput input) : SV_Target
{
	float hardwareDepth = DepthTexture.Load(int3(input.screenSpacePos.xy, 0)).r;
	float4 worldSpacePos = ComputeWorldSpacePosition(input.texCoord, hardwareDepth, g_Transform.viewProjInvMatrix);
	
	int3 gridCell = ComputeGridCell(g_GridConfig, worldSpacePos.xyz);
	if (all(float3(-1.0f, -1.0f, -1.0f) < gridCell.xyz) && all(gridCell.xyz < g_GridConfig.numCells.xyz))
	{
		int cellIndex = ComputeCellIndex(g_GridConfig, gridCell);

		float3 color = GridBuffer[cellIndex].colorAndNumOccluders.rgb;
		return float4(color, 1.0f);
	}

	return float4(0.7f, 0.8f, 0.5f, 1.0f);;
}