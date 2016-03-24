#include "Reconstruction.hlsl"

struct PointLightGeometry
{
	float3 worldSpacePos;
	float range;
};

struct PointLightProps
{
	float4 color;
};

struct SpotLightGeometry
{
	float3 worldSpacePos;
	float range;
};

struct SpotLightProps
{
	float4 color;
};

float4 CreatePlanePassingThroughOrigin(float3 point1, float3 point2)
{
	float3 planeNormal = cross(point1, point2);
	float signedDistFromOrigin = 0.0f;

	return float4(planeNormal, signedDistFromOrigin);
}

Texture2D g_DepthTexture : register(t0);

#define TILE_SIZE						16
#define NUM_THREADS_PER_TILE		   (TILE_SIZE * TILE_SIZE)
#define MAX_NUM_POINT_LIGHTS_PER_TILE	128
#define MAX_NUM_SPOT_LIGHTS_PER_TILE	128

groupshared uint g_ViewSpaceMinDepthIntPerTile;
groupshared uint g_ViewSpaceMaxDepthIntPerTile;

groupshared uint g_NumVisiblePointLightsPerTile;
groupshared uint g_VisiblePointLightIndicesPerTile[MAX_NUM_POINT_LIGHTS_PER_TILE];

groupshared uint g_NumVisibleSpotLightsPerTile;
groupshared uint g_VisibleSpotLightIndicesPerTile[MAX_NUM_SPOT_LIGHTS_PER_TILE];

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void Main(uint3 globalThreadId : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint localThreadIndex : SV_GroupIndex)
{
	float hardwareDepth = g_DepthTexture.Load(int3(globalThreadId.xy, 0)).r;
	float viewSpaceDepth = ComputeViewSpaceDepth(hardwareDepth, g_Transform.projMatrix);
	
	if (localThreadIndex == 0)
	{
		g_ViewSpaceMinDepthIntPerTile = 0x7F7FFFFF;
		g_ViewSpaceMaxDepthIntPerTile = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	if (hardwareDepth != 1.0f)
	{
		uint viewSpaceDepthInt = asuint(viewSpaceDepth);

		InterlockedMin(g_ViewSpaceMinDepthIntPerTile, viewSpaceDepthInt);
		InterlockedMax(g_ViewSpaceMaxDepthIntPerTile, viewSpaceDepthInt);
	}
	GroupMemoryBarrierWithGroupSync();

	float viewSpaceMinDepthPerTile = asfloat(g_ViewSpaceMinDepthIntPerTile);
	float viewSpaceMaxDepthPerTile = asfloat(g_ViewSpaceMaxDepthIntPerTile);
	
	uint2 screenSpaceTileTopLeftCorner = groupId.xy * TILE_SIZE;
	uint2 screenSpaceTileBottomRightCorner = screenSpaceTileTopLeftCorner.xy + TILE_SIZE;
}