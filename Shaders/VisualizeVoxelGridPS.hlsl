#include "Reconstruction.hlsl"
#include "VoxelGrid.hlsl"

#define VOXEL_DATA_TYPE_DIFFUSE_COLOR		0
#define VOXEL_DATA_TYPE_NORMAL				1

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
	if (all(int3(-1, -1, -1) < gridCell.xyz) && all(gridCell.xyz < g_GridConfig.numCells.xyz))
	{
		int cellIndex = ComputeGridCellIndex(g_GridConfig, gridCell);

#if (VOXEL_DATA_TYPE == VOXEL_DATA_TYPE_DIFFUSE_COLOR)
		float3 diffuseColor = GridBuffer[cellIndex].diffuseColor;
		return float4(diffuseColor, 1.0f);
#endif // VOXEL_DATA_TYPE_DIFFUSE_COLOR

#if (VOXEL_DATA_TYPE == VOXEL_DATA_TYPE_NORMAL)
		float3 worldSpaceNormal = GridBuffer[cellIndex].worldSpaceNormal;
		return float4(0.5f * worldSpaceNormal + 0.5f, 1.0f);
#endif // VOXEL_DATA_TYPE_NORMAL
	}

	return float4(0.7f, 0.8f, 0.5f, 1.0f);
}