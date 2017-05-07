#include "Foundation.hlsl"
#include "VoxelGrid.hlsl"
#include "SphericalHarmonics.hlsl"

#define WORLD_SPACE_X		1
#define WORLD_SPACE_Y		2
#define WORLD_SPACE_Z		3

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
#if VIEW_DIRECTION == WORLD_SPACE_Z
	float2 clipSpaceCellSize = 2.0f / float2(NUM_GRID_CELLS_X, NUM_GRID_CELLS_Y);
#endif // WORLD_SPACE_Z

#if VIEW_DIRECTION == WORLD_SPACE_Y
	float2 clipSpaceCellSize = 2.0f / float2(NUM_GRID_CELLS_X, NUM_GRID_CELLS_Z);
#endif // WORLD_SPACE_Y

#if VIEW_DIRECTION == WORLD_SPACE_X
	float2 clipSpaceCellSize = 2.0f / float2(NUM_GRID_CELLS_Z, NUM_GRID_CELLS_Y);
#endif // WORLD_SPACE_X
				
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
#if VIEW_DIRECTION == WORLD_SPACE_Z
	float2 clipSpaceCellSize = 2.0f / float2(NUM_GRID_CELLS_X, NUM_GRID_CELLS_Y);
#endif // WORLD_SPACE_Z

#if VIEW_DIRECTION == WORLD_SPACE_Y
	float2 clipSpaceCellSize = 2.0f / float2(NUM_GRID_CELLS_X, NUM_GRID_CELLS_Z);
#endif // WORLD_SPACE_Y

#if VIEW_DIRECTION == WORLD_SPACE_X
	float2 clipSpaceCellSize = 2.0f / float2(NUM_GRID_CELLS_Z, NUM_GRID_CELLS_Y);
#endif // WORLD_SPACE_X

	VSOutput output;
	output.clipSpacePos = float4(g_ClipSpaceLeftBoundary[vertexId] + float2(float(boundaryId) * clipSpaceCellSize.x, 0.0f), 0.0f, 1.0f);
	output.color = float4(0.0f, 1.0f, 0.0f, 1.0f);

	return output;
}
#endif // VERTICAL_BOUNDARY

#if INTENSITY_DISTRIBUTION == 1 
static const float g_AngleSubtendedByIntensityPolygonSide = g_TwoPI / float(NUM_INTENSITY_POLYGON_SIDES);
static const float g_VisualizedIntensityScale = 4.0f;

VSOutput Main(uint cellFlattenedId : SV_InstanceID, uint vertexId : SV_VertexID)
{
	float vertexAngle = float(vertexId) * g_AngleSubtendedByIntensityPolygonSide;

#if VIEW_DIRECTION == WORLD_SPACE_Z
	uint numCellsInGridRow = NUM_GRID_CELLS_X;
	uint3 texturePos = uint3(cellFlattenedId % numCellsInGridRow, cellFlattenedId / numCellsInGridRow, SLICE_TO_VISUALIZE);

	float2 clipSpaceCellHalfSize = 1.0f / float2(NUM_GRID_CELLS_X, NUM_GRID_CELLS_Y);

	float3 worldSpaceVertexOnUnitSphere = float3(0.0f, 0.0f, 0.0f);
	sincos(vertexAngle, worldSpaceVertexOnUnitSphere.y, worldSpaceVertexOnUnitSphere.x);

	float2 clipSpaceIntensityDir = worldSpaceVertexOnUnitSphere.xy;
#endif // WORLD_SPACE_Z

#if VIEW_DIRECTION == WORLD_SPACE_Y
	uint numCellsInGridRow = NUM_GRID_CELLS_X;
#endif // WORLD_SPACE_Y

#if VIEW_DIRECTION == WORLD_SPACE_X
	uint numCellsInGridRow = NUM_GRID_CELLS_Z;
#endif // WORLD_SPACE_X
	
	float2 clipSpaceCellCenter = float2(-1.0f, 1.0f) + float2(1 + 2 * texturePos.x, 1 + 2 * texturePos.y) * float2(clipSpaceCellHalfSize.x, -clipSpaceCellHalfSize.y);
	
	float4 intensityCoeffs = g_IntensityCoeffsTextures[g_TextureIndex][texturePos];
	float intensity = dot(intensityCoeffs, SH(worldSpaceVertexOnUnitSphere));
	
	float2 clipSpaceVertexScale = (g_VisualizedIntensityScale * intensity) * clipSpaceCellHalfSize;
	float2 clipSpaceVertexOffset = clipSpaceVertexScale * clipSpaceIntensityDir;

	VSOutput output;
	output.clipSpacePos = float4(clipSpaceCellCenter + clipSpaceVertexOffset, 0.0f, 1.0f);
	output.color = float4(0.0f, 1.0f, 1.0f, 1.0f);

	return output;
}
#endif // INTENSITY_DISTRIBUTION
