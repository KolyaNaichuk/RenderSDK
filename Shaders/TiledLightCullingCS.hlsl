#include "Reconstruction.hlsl"

#ifdef USE_POINT_LIGHTS
struct PointLightGeometry
{
	float3 viewSpaceCenter;
	float radius;
};

struct PointLightProps
{
	float4 color;
};
#endif // USE_POINT_LIGHTS

#ifdef USE_SPOT_LIGHTS
struct SpotLightGeometry
{
	float3 viewSpaceCenter;
	float radius;
};

struct SpotLightProps
{
	float4 color;
};
#endif // USE_SPOT_LIGHTS

float4 CreatePlanePassingThroughOrigin(float3 pt1, float3 pt2)
{
	float3 planeNormal = cross(pt1, pt2);
	float signedDistFromOrigin = 0.0f;

	return float4(planeNormal, signedDistFromOrigin);
}

float SignedDistanceToPoint(float4 plane, float3 pt)
{
	return dot(plane, float4(pt, 1.0f));
}

Texture2D g_DepthTexture : register(t0);

#ifdef USE_POINT_LIGHTS
StructuredBuffer<PointLightGeometry> g_PointLightGeometryBuffer : register(t1);
#endif // USE_POINT_LIGHTS

#ifdef USE_SPOT_LIGHTS
StructuredBuffer<SpotLightGeometry> g_SpotLightGeometryBuffer : register(t2);
#endif // USE_SPOT_LIGHTS

#define TILE_SIZE						16
#define NUM_THREADS_PER_TILE		   (TILE_SIZE * TILE_SIZE)
#define MAX_NUM_LIGHTS_PER_TILE			256

groupshared uint g_ViewSpaceMinDepthIntPerTile;
groupshared uint g_ViewSpaceMaxDepthIntPerTile;

#ifdef USE_POINT_LIGHTS
groupshared uint g_NumPointLightsPerTile;
groupshared uint g_PointLightIndicesPerTile[MAX_NUM_LIGHTS_PER_TILE];
#endif // USE_POINT_LIGHTS

#ifdef USE_SPOT_LIGHTS
groupshared uint g_NumSpotLightsPerTile;
groupshared uint g_SpotLightIndicesPerTile[MAX_NUM_LIGHTS_PER_TILE];
#endif // USE_SPOT_LIGHTS

bool TestSphereAgainstFrustum(float4 frustumSidePlanes[4], float frustumNearPlaneDepth,
	float frustumFarPlaneDepth, float3 sphereCenter, float sphereRadius)
{
	return (SignedDistanceToPoint(frustumSidePlanes[0], sphereCenter) < sphereRadius) &&
		(SignedDistanceToPoint(frustumSidePlanes[1], sphereCenter) < sphereRadius) &&
		(SignedDistanceToPoint(frustumSidePlanes[2], sphereCenter) < sphereRadius) &&
		(SignedDistanceToPoint(frustumSidePlanes[3], sphereCenter) < sphereRadius);
}

#ifdef USE_POINT_LIGHTS
void FindPointLightsPerTile(float4 viewSpaceFrustumPlanes[4])
{
	for (uint lightIndex = localThreadIndex; lightIndex < g_NumPointLights; lightIndex += NUM_THREADS_PER_TILE)
	{
		float3 lightViewSpacePos = g_PointLightGeometryBuffer[lightIndex].viewSpacePos;
		float lightRadius = g_PointLightGeometryBuffer[lightIndex].radius;

		bool insideOrOverlaps = 


		if (insideOrOverlaps)
		{
			uint listIndex;
			InterlockedAdd(g_NumPointLightsPerTile, 1, listIndex);
			g_PointLightIndicesPerTile[listIndex] = lightIndex;
		}
	}
}
#endif // USE_POINT_LIGHTS

#ifdef USE_SPOT_LIGHTS
void FindSpotLightsPerTile()
{
	for (uint lightIndex = localThreadIndex; lightIndex < g_NumSpotLights; lightIndex += NUM_THREADS_PER_TILE)
	{
		SpotLightGeometry lightGeometry = g_SpotLightGeometryBuffer[lightIndex];

		bool insideOrOverlaps = true;
		for ()
		
		if (insideOrOverlaps)
		{
			uint listIndex;
			InterlockedAdd(g_NumSpotLightsPerTile, 1, listIndex);
			g_SpotLightIndicesPerTile[listIndex] = lightIndex;
		}
	}
}
#endif // USE_SPOT_LIGHTS

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void Main(uint3 globalThreadId : SV_DispatchThreadID, uint3 groupId : SV_GroupID, uint localThreadIndex : SV_GroupIndex)
{
	if (localThreadIndex == 0)
	{
		g_ViewSpaceMinDepthIntPerTile = 0x7F7FFFFF;
		g_ViewSpaceMaxDepthIntPerTile = 0;

#ifdef USE_POINT_LIGHTS
		g_NumPointLightsPerTile = 0;
#endif // USE_POINT_LIGHTS

#ifdef USE_SPOT_LIGHTS
		g_NumSpotLightsPerTile = 0;
#endif // USE_SPOT_LIGHTS
	}
	GroupMemoryBarrierWithGroupSync();

	float hardwareDepth = g_DepthTexture.Load(int3(globalThreadId.xy, 0)).r;
	float viewSpaceDepth = ComputeViewSpaceDepth(hardwareDepth, g_Transform.projMatrix);

	if (hardwareDepth != 1.0f)
	{
		uint viewSpaceDepthInt = asuint(viewSpaceDepth);

		InterlockedMin(g_ViewSpaceMinDepthIntPerTile, viewSpaceDepthInt);
		InterlockedMax(g_ViewSpaceMaxDepthIntPerTile, viewSpaceDepthInt);
	}
	GroupMemoryBarrierWithGroupSync();

	float viewSpaceMinDepthPerTile = asfloat(g_ViewSpaceMinDepthIntPerTile);
	float viewSpaceMaxDepthPerTile = asfloat(g_ViewSpaceMaxDepthIntPerTile);
	
	uint2 screenSpaceTileTLCorner = groupId.xy * TILE_SIZE;
	uint2 screenSpaceTileBRCorner = screenSpaceTileTLCorner.xy + TILE_SIZE;

	float2 texSpaceTileTLCorner = (float2(screenSpaceTileTLCorner.xy) + 0.5f) * g_RcpScreenSize;
	float2 texSpaceTileBRCorner = (float2(screenSpaceTileBRCorner.xy) + 0.5f) * g_RcpScreenSize;
	float2 texSpaceTileTRCorner =  float2(texSpaceTileBRCorner.x, texSpaceTileTLCorner.y);
	float2 texSpaceTileBLCorner =  float2(texSpaceTileTLCorner.x, texSpaceTileBRCorner.y);

	float3 viewSpaceTileTLCorner = ComputeViewSpacePosition(texSpaceTileTLCorner, 1.0f, g_ProjInvMatrix).xyz;
	float3 viewSpaceTileTRCorner = ComputeViewSpacePosition(texSpaceTileTRCorner, 1.0f, g_ProjInvMatrix).xyz;
	float3 viewSpaceTileBLCorner = ComputeViewSpacePosition(texSpaceTileBLCorner, 1.0f, g_ProjInvMatrix).xyz;
	float3 viewSpaceTileBRCorner = ComputeViewSpacePosition(texSpaceTileBRCorner, 1.0f, g_ProjInvMatrix).xyz;

	float4 viewSpaceFrusumPlanes[4];
	viewSpaceFrusumPlanes[0] = CreatePlanePassingThroughOrigin(texSpaceTileTRCorner, texSpaceTileTLCorner);
	viewSpaceFrusumPlanes[1] = CreatePlanePassingThroughOrigin(texSpaceTileBLCorner, texSpaceTileBRCorner);
	viewSpaceFrusumPlanes[2] = CreatePlanePassingThroughOrigin(texSpaceTileTLCorner, texSpaceTileBLCorner);
	viewSpaceFrusumPlanes[3] = CreatePlanePassingThroughOrigin(texSpaceTileBRCorner, texSpaceTileTRCorner);
	
#ifdef USE_POINT_LIGHTS
	FindPointLightsPerTile(viewSpaceFrusumPlanes);
	GroupMemoryBarrierWithGroupSync();
#endif // USE_POINT_LIGHTS

#ifdef USE_SPOT_LIGHTS
	FindSpotLightsPerTile(viewSpaceFrusumPlanes);
	GroupMemoryBarrierWithGroupSync();
#endif // USE_SPOT_LIGHTS
}
