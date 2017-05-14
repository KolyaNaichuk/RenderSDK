#ifndef __VOXEL_GRID__
#define __VOXEL_GRID__

struct Voxel
{
	float numOccluders;
	float3 diffuseColor;
	float3 worldSpaceNormal;
};

struct GridConfig
{
	float4 worldSpaceOrigin;
	float4 size;
	float4 rcpSize;
	float4 cellSize;
	float4 rcpCellSize;
	int4   numCells;
	float4 rcpNumCells;
	float  fluxWeight;
	float  blockerPotentialValue;
	float2 notUsed1;
	float4 notUsed2[8];
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
	float3 gridCell = gridSpacePos.xyz * gridConfig.rcpCellSize.xyz;
	return int3(gridCell);
}

float3 ComputeWorldSpacePosition(GridConfig gridConfig, int3 gridCell)
{
	float3 gridSpaceOffset = float3(gridCell) * gridConfig.cellSize.xyz;
	return gridConfig.worldSpaceOrigin.xyz + gridSpaceOffset;
}

int ComputeGridCellIndex(GridConfig gridConfig, int3 gridCell)
{
	return gridCell.x + gridCell.y * gridConfig.numCells.x + gridCell.z * gridConfig.numCells.x * gridConfig.numCells.y;
}

uint3 ComputeTexturePosition(GridConfig gridConfig, int3 gridCell)
{
	return uint3(gridCell.x, gridConfig.numCells.y - 1 - gridCell.y, gridCell.z);
}

bool IsCellOutsideGrid(GridConfig gridConfig, int3 gridCell)
{
	return (gridCell.x > gridConfig.numCells.x - 1) || (gridCell.x < 0) ||
		(gridCell.y > gridConfig.numCells.y - 1) || (gridCell.y < 0) ||
		(gridCell.z > gridConfig.numCells.z - 1) || (gridCell.z < 0);
}

#endif