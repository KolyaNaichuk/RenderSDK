#include "Foundation.hlsl"
#include "VoxelGrid.hlsl"
#include "SphericalHarmonics.hlsl"

#define VIEW_DIRECTION_Z		1
#define VIEW_DIRECTION_Y		2
#define VIEW_DIRECTION_X		3

struct VSOutput
{
	float4 clipSpacePos : SV_Position;
	float4 color		: COLOR;
};

cbuffer TextureIndexBuffer : register(b0)
{
	uint g_TextureIndex;
}

Texture3D<float4> g_IntensityCoeffsTextures[9] : register(t0);

#if HORIZONTAL_BOUNDARY == 1
static const float2 g_ClipSpaceTopBoundary[2] =
{
	float2(-1.0f, 1.0f),
	float2( 1.0f, 1.0f)
};

VSOutput Main(uint boundaryId : SV_InstanceID, uint vertexId : SV_VertexID)
{
#if VIEW_DIRECTION == VIEW_DIRECTION_Z
	float2 clipSpaceCellSize = 2.0f / float2(NUM_GRID_CELLS_X, NUM_GRID_CELLS_Y);
#endif // VIEW_DIRECTION_Z

#if VIEW_DIRECTION == VIEW_DIRECTION_Y
	float2 clipSpaceCellSize = 2.0f / float2(NUM_GRID_CELLS_X, NUM_GRID_CELLS_Z);
#endif // VIEW_DIRECTION_Y

#if VIEW_DIRECTION == VIEW_DIRECTION_X
	float2 clipSpaceCellSize = 2.0f / float2(NUM_GRID_CELLS_Z, NUM_GRID_CELLS_Y);
#endif // VIEW_DIRECTION_X
				
	VSOutput output;
	output.clipSpacePos = float4(g_ClipSpaceTopBoundary[vertexId] - float2(0.0f, float(boundaryId) * clipSpaceCellSize.y), 0.0f, 1.0f);
	output.color = float4(0.0f, 1.0f, 0.0f, 1.0f);
	
	return output;
}
#endif // HORIZONTAL_BOUNDARY

#if VERTICAL_BOUNDARY == 1
static const float2 g_ClipSpaceLeftBoundary[2] =
{
	float2(-1.0f,  1.0f),
	float2(-1.0f, -1.0f)
};

VSOutput Main(uint boundaryId : SV_InstanceID, uint vertexId : SV_VertexID)
{
#if VIEW_DIRECTION == VIEW_DIRECTION_Z
	float2 clipSpaceCellSize = 2.0f / float2(NUM_GRID_CELLS_X, NUM_GRID_CELLS_Y);
#endif // VIEW_DIRECTION_Z

#if VIEW_DIRECTION == VIEW_DIRECTION_Y
	float2 clipSpaceCellSize = 2.0f / float2(NUM_GRID_CELLS_X, NUM_GRID_CELLS_Z);
#endif // VIEW_DIRECTION_Y

#if VIEW_DIRECTION == VIEW_DIRECTION_X
	float2 clipSpaceCellSize = 2.0f / float2(NUM_GRID_CELLS_Z, NUM_GRID_CELLS_Y);
#endif // VIEW_DIRECTION_X

	VSOutput output;
	output.clipSpacePos = float4(g_ClipSpaceLeftBoundary[vertexId] + float2(float(boundaryId) * clipSpaceCellSize.x, 0.0f), 0.0f, 1.0f);
	output.color = float4(0.0f, 1.0f, 0.0f, 1.0f);

	return output;
}
#endif // VERTICAL_BOUNDARY

#if INTENSITY_DISTRIBUTION == 1 
static const float g_AngleSubtendedByIntensityPolygonSide = g_TwoPI / float(NUM_INTENSITY_POLYGON_SIDES);
static const float g_VisualizedIntensityScale = 2.5f;

VSOutput Main(uint cellFlattenedId : SV_InstanceID, uint vertexId : SV_VertexID)
{
	float vertexAngle = float(vertexId) * g_AngleSubtendedByIntensityPolygonSide;

#if VIEW_DIRECTION == VIEW_DIRECTION_Z
	uint numCellsInGridRow = NUM_GRID_CELLS_X;
	uint3 cellId = uint3(cellFlattenedId % numCellsInGridRow, cellFlattenedId / numCellsInGridRow, SLICE_TO_VISUALIZE);

	float2 clipSpaceCellHalfSize = 1.0f / float2(NUM_GRID_CELLS_X, NUM_GRID_CELLS_Y);

	float3 vertexDirOnUnitSphere = float3(0.0f, 0.0f, 0.0f);
	sincos(vertexAngle, vertexDirOnUnitSphere.y, vertexDirOnUnitSphere.x);

	float2 visualizedIntensityDir = vertexDirOnUnitSphere.xy;
#endif // VIEW_DIRECTION_Z

#if VIEW_DIRECTION == VIEW_DIRECTION_Y
	uint numCellsInGridRow = NUM_GRID_CELLS_X;
	uint3 cellId = uint3(cellFlattenedId % numCellsInGridRow, SLICE_TO_VISUALIZE, cellFlattenedId / numCellsInGridRow);

	float2 clipSpaceCellHalfSize = 1.0f / float2(NUM_GRID_CELLS_X, NUM_GRID_CELLS_Z);

	float3 vertexDirOnUnitSphere = float3(0.0f, 0.0f, 0.0f);
	sincos(vertexAngle, vertexDirOnUnitSphere.z, vertexDirOnUnitSphere.x);

	float2 visualizedIntensityDir = vertexDirOnUnitSphere.xz;
#endif // VIEW_DIRECTION_Y

#if VIEW_DIRECTION == VIEW_DIRECTION_X
	uint numCellsInGridRow = NUM_GRID_CELLS_Z;
	uint3 cellId = uint3(SLICE_TO_VISUALIZE, cellFlattenedId / numCellsInGridRow, cellFlattenedId % numCellsInGridRow);

	float2 clipSpaceCellHalfSize = 1.0f / float2(NUM_GRID_CELLS_Z, NUM_GRID_CELLS_Y);

	float3 vertexDirOnUnitSphere = float3(0.0f, 0.0f, 0.0f);
	sincos(vertexAngle, vertexDirOnUnitSphere.z, vertexDirOnUnitSphere.y);

	float2 visualizedIntensityDir = vertexDirOnUnitSphere.zy;
#endif // VIEW_DIRECTION_X
	
	float2 clipSpaceCellCenter = float2(-1.0f, 1.0f) + float2(1 + 2 * cellId.x, 1 + 2 * cellId.y) * float2(clipSpaceCellHalfSize.x, -clipSpaceCellHalfSize.y);
	
	float4 intensityCoeffs = g_IntensityCoeffsTextures[g_TextureIndex][cellId];
	float intensity = dot(intensityCoeffs, SH(vertexDirOnUnitSphere));
	
	float2 clipSpaceVertexScale = (g_VisualizedIntensityScale * intensity) * clipSpaceCellHalfSize;
	float2 clipSpaceVertexOffset = clipSpaceVertexScale * visualizedIntensityDir;

	VSOutput output;
	output.clipSpacePos = float4(clipSpaceCellCenter + clipSpaceVertexOffset, 0.0f, 1.0f);
	output.color = float4(0.0f, 1.0f, 1.0f, 1.0f);

	return output;
}
#endif // INTENSITY_DISTRIBUTION
