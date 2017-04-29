#include "Foundation.hlsl"
#include "VoxelGrid.hlsl"
#include "SphericalHarmonics.hlsl"

#define RENDER_SLICE_XY		1
#define RENDER_SLICE_XZ		2
#define RENDER_SLICE_ZY		3

struct VSOutput
{
	float4 clipSpacePos : SV_Position;
	float4 color		: COLOR;
};

cbuffer GridConfigDataBuffer : register(b0)
{
	GridConfig g_GridConfig;
}

Texture3D g_IntensityCoeffsTexture : register(t0);

#if RENDER_CELL_VERTICAL_BOUNDARY == 1
static const float2 g_ClipSpaceTopBoundary[2] =
{
	float2(-1.0f, 1.0f),
	float2( 1.0f, 1.0f)
};

VSOutput Main(uint boundaryId : SV_InstanceID, uint vertexId : SV_VertexID)
{
#if RENDER_SLICE == RENDER_SLICE_XY
	float2 clipSpaceCellSize = float2(2.0f, 2.0f) * g_GridConfig.rcpNumCells.xy;
#endif // RENDER_SLICE_XY

#if RENDER_SLICE == RENDER_SLICE_XZ
	float2 clipSpaceCellSize = float2(2.0f, 2.0f) * g_GridConfig.rcpNumCells.xz;
#endif // RENDER_SLICE_XZ

#if RENDER_SLICE == RENDER_SLICE_ZY
	float2 clipSpaceCellSize = float2(2.0f, 2.0f) * g_GridConfig.rcpNumCells.zy;
#endif // RENDER_SLICE_ZY
				
	VSOutput output;
	output.clipSpacePos = float4(g_ClipSpaceTopBoundary[vertexId] - float2(0.0f, float(boundaryId) * clipSpaceCellSize.y), 0.0f, 1.0f);
	output.color = float4(0.0f, 1.0f, 0.0f, 1.0f);
	
	return output;
}
#endif // RENDER_CELL_VERTICAL_BOUNDARY

#if RENDER_CELL_HORIZONTAL_BOUNDARY == 1
static const float2 g_ClipSpaceLeftBoundary[2] =
{
	float2(-1.0f,  1.0f),
	float2(-1.0f, -1.0f)
};

VSOutput Main(uint boundaryId : SV_InstanceID, uint vertexId : SV_VertexID)
{
#if RENDER_SLICE == RENDER_SLICE_XY
	float2 clipSpaceCellSize = float2(2.0f, 2.0f) * g_GridConfig.rcpNumCells.xy;
#endif // RENDER_SLICE_XY

#if RENDER_SLICE == RENDER_SLICE_XZ
	float2 clipSpaceCellSize = float2(2.0f, 2.0f) * g_GridConfig.rcpNumCells.xz;
#endif // RENDER_SLICE_XZ

#if RENDER_SLICE == RENDER_SLICE_ZY
	float2 clipSpaceCellSize = float2(2.0f, 2.0f) * g_GridConfig.rcpNumCells.zy;
#endif // RENDER_SLICE_ZY

	VSOutput output;
	output.clipSpacePos = float4(g_ClipSpaceLeftBoundary[vertexId] + float2(float(boundaryId) * clipSpaceCellSize.x, 0.0f), 0.0f, 1.0f);
	output.color = float4(0.0f, 1.0f, 0.0f, 1.0f);

	return output;
}
#endif // RENDER_CELL_HORIZONTAL_BOUNDARY

#if RENDER_CELL_INTENSITY_DISTRIBUTION == 1 
static const float g_AngleSubtendedByPolygonSide = g_TwoPI / float(NUM_POLYGON_SIDES);
static const float g_VisualizedIntensityScale = 0.9f;

VSOutput Main(uint cellFlattenedId : SV_InstanceID, uint vertexId : SV_VertexID)
{
	float vertexAngle = float(vertexId) * g_AngleSubtendedByPolygonSide;

#if RENDER_SLICE == RENDER_SLICE_XY
	float2 clipSpaceCellHalfSize = g_GridConfig.rcpNumCells.xy;

	float3 vertexDir = float3(0.0f, 0.0f, 0.0f);
	sincos(vertexAngle, vertexDir.y, vertexDir.x);
#endif // RENDER_SLICE_XY

#if RENDER_SLICE == RENDER_SLICE_XZ
	float2 clipSpaceCellHalfSize = g_GridConfig.rcpNumCells.xz;

	float3 vertexDir = float3(0.0f, 0.0f, 0.0f);
	sincos(vertexAngle, vertexDir.z, vertexDir.x);
#endif // RENDER_SLICE_XZ

#if RENDER_SLICE == RENDER_SLICE_ZY
	float2 clipSpaceCellHalfSize = g_GridConfig.rcpNumCells.zy;

	float3 vertexDir = float3(0.0f, 0.0f, 0.0f);
	sincos(vertexAngle, vertexDir.z, vertexDir.y);
#endif // RENDER_SLICE_ZY
	
	float3 vertexDirNorm = normalize(vertexDir);

	float4 intensityCoeffs = g_IntensityCoeffsTexture[cellId];
	float intensity = dot(intensityCoeffs, SH(vertexDirNorm));
	
	float2 clipSpaceVertexScale = (g_VisualizedIntensityScale / intensity) * clipSpaceCellHalfSize;
	float2 clipSpaceVertexOffset = clipSpaceVertexScale * vertexDirNorm;

	VSOutput output;
	output.clipSpacePos = float4(clipSpaceCellCenter + clipSpaceVertexOffset, 0.0f, 1.0f);
	output.color = float4(0.0f, 1.0f, 1.0f, 1.0f);

	return output;
}
#endif // RENDER_CELL_INTENSITY_DISTRIBUTION
