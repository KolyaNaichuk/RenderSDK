#ifndef __VOXEL_GRID__
#define __VOXEL_GRID__

struct Voxel
{
	float4 colorAndNumOccluders;
};

struct GridConfig
{
	float4 worldSpaceOrigin;
	float4 cellSize;
	float4 rcpCellSize;
	int4   numCells;
	float4 notUsed[12];
};

struct ObjectTransform
{
	matrix worldPositionMatrix;
	matrix worldNormalMatrix;
	matrix worldViewProjMatrix;
	float4 notUsed[4];
};

struct CameraTransform
{
	matrix viewProjInvMatrix;
	matrix viewProjMatrices[3];
};

int3 ComputeGridCell(GridConfig gridConfig, float3 worldSpacePos)
{
	float3 gridSpacePos = worldSpacePos - gridConfig.worldSpaceOrigin.xyz;
	float3 gridCell = round(gridSpacePos.xyz * gridConfig.rcpCellSize.xyz);

	return int3(gridCell.x, gridCell.y, gridCell.z);
}

float3 ComputeWorldSpacePosition(GridConfig gridConfig, int3 gridCell)
{
	float3 gridSpaceOffset = gridCell * gridConfig.cellSize.xyz;
	return gridConfig.worldSpaceOrigin.xyz + gridSpaceOffset;
}

int ComputeGridCellIndex(GridConfig gridConfig, int3 gridCell)
{
	return gridCell.x + gridCell.y * gridConfig.numCells.x + gridCell.z * gridConfig.numCells.x * gridConfig.numCells.y;
}

#endif