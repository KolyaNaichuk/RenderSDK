#include "Foundation.hlsl"
#include "OverlapTest.hlsl"
#include "Reconstruction.hlsl"

#define NUM_THREADS_PER_TILE	(TILE_SIZE * TILE_SIZE)

cbuffer AppDataBuffer : register(b0)
{
	AppData g_AppData;
}

cbuffer Constants32BitBuffer : register(b1)
{
	uint g_NumPointLights;
	uint g_NumSpotLights;
}

Texture2D g_DepthTexture : register(t0);

#if MAX_NUM_POINT_LIGHTS > 0
StructuredBuffer<Sphere> g_PointLightWorldBoundsBuffer : register(t1);
RWBuffer<uint> g_PointLightIndicesOffsetBuffer : register(u0);
RWBuffer<uint> g_PointLightIndexPerTileBuffer : register(u1);
RWStructuredBuffer<Range> g_PointLightRangePerTileBuffer : register(u2);

groupshared uint g_NumPointLightsPerTile;
groupshared uint g_PointLightIndicesPerTile[MAX_NUM_POINT_LIGHTS];
groupshared uint g_PointLightIndicesOffset;
#endif

#if MAX_NUM_SPOT_LIGHTS > 0
StructuredBuffer<Sphere> g_SpotLightWorldBoundsBuffer : register(t2);
RWBuffer<uint> g_SpotLightIndicesOffsetBuffer : register(u3);
RWBuffer<uint> g_SpotLightIndexPerTileBuffer : register(u4);
RWStructuredBuffer<Range> g_SpotLightRangePerTileBuffer : register(u5);

groupshared uint g_NumSpotLightsPerTile;
groupshared uint g_SpotLightIndicesPerTile[MAX_NUM_SPOT_LIGHTS];
groupshared uint g_SpotLightIndicesOffset;
#endif

groupshared uint g_ViewSpaceMinDepthIntPerTile;
groupshared uint g_ViewSpaceMaxDepthIntPerTile;

float4 CreatePlaneThroughOrigin(float3 point1, float3 point2)
{
	float3 planeNormal = normalize(cross(point1, point2));
	float signedDistFromOrigin = 0.0f;

	return float4(planeNormal, signedDistFromOrigin);
}

void BuildFrustumSidePlanes(out float4 viewSpaceFrusumSidePlanes[4], uint2 tileId)
{
	uint2 screenSpaceTileTLCorner = tileId.xy * TILE_SIZE;
	uint2 screenSpaceTileBRCorner = screenSpaceTileTLCorner.xy + TILE_SIZE;

	float2 texSpaceTileTLCorner = (float2(screenSpaceTileTLCorner.xy) + 0.5f) * g_AppData.rcpScreenSize;
	float2 texSpaceTileBRCorner = (float2(screenSpaceTileBRCorner.xy) + 0.5f) * g_AppData.rcpScreenSize;
	float2 texSpaceTileTRCorner =  float2(texSpaceTileBRCorner.x, texSpaceTileTLCorner.y);
	float2 texSpaceTileBLCorner =  float2(texSpaceTileTLCorner.x, texSpaceTileBRCorner.y);

	float3 viewSpaceTileTLCorner = ComputeViewSpacePosition(texSpaceTileTLCorner, 1.0f, g_AppData.projInvMatrix).xyz;
	float3 viewSpaceTileTRCorner = ComputeViewSpacePosition(texSpaceTileTRCorner, 1.0f, g_AppData.projInvMatrix).xyz;
	float3 viewSpaceTileBLCorner = ComputeViewSpacePosition(texSpaceTileBLCorner, 1.0f, g_AppData.projInvMatrix).xyz;
	float3 viewSpaceTileBRCorner = ComputeViewSpacePosition(texSpaceTileBRCorner, 1.0f, g_AppData.projInvMatrix).xyz;

	viewSpaceFrusumSidePlanes[0] = CreatePlaneThroughOrigin(viewSpaceTileTRCorner, viewSpaceTileTLCorner);
	viewSpaceFrusumSidePlanes[1] = CreatePlaneThroughOrigin(viewSpaceTileBLCorner, viewSpaceTileBRCorner);
	viewSpaceFrusumSidePlanes[2] = CreatePlaneThroughOrigin(viewSpaceTileTLCorner, viewSpaceTileBLCorner);
	viewSpaceFrusumSidePlanes[3] = CreatePlaneThroughOrigin(viewSpaceTileBRCorner, viewSpaceTileTRCorner);
}

bool TestSphereAgainstFrustum(float4 frustumSidePlanes[4], float frustumMinZ, float frustumMaxZ, Sphere sphere)
{
	bool insideOrOverlap = TestSphereAgainstPlane(frustumSidePlanes[0], sphere) &&
		TestSphereAgainstPlane(frustumSidePlanes[1], sphere) &&
		TestSphereAgainstPlane(frustumSidePlanes[2], sphere) &&
		TestSphereAgainstPlane(frustumSidePlanes[3], sphere) &&
		(frustumMinZ < sphere.center.z + sphere.radius) &&
		(frustumMaxZ > sphere.center.z - sphere.radius);

	return insideOrOverlap;
}

#if MAX_NUM_POINT_LIGHTS > 0
void CullPointLightsPerTile(uint localThreadIndex, float4 viewSpaceFrustumSidePlanes[4], float viewSpaceMinDepth, float viewSpaceMaxDepth)
{
	for (uint lightIndex = localThreadIndex; lightIndex < g_NumPointLights; lightIndex += NUM_THREADS_PER_TILE)
	{
		Sphere viewSpaceLightBounds = g_PointLightWorldBoundsBuffer[lightIndex];
		viewSpaceLightBounds.center = mul(g_AppData.viewMatrix, float4(viewSpaceLightBounds.center.xyz, 1.0f)).xyz;

		if (TestSphereAgainstFrustum(viewSpaceFrustumSidePlanes, viewSpaceMinDepth, viewSpaceMaxDepth, viewSpaceLightBounds))
		{
			uint listIndex;
			InterlockedAdd(g_NumPointLightsPerTile, 1, listIndex);
			g_PointLightIndicesPerTile[listIndex] = lightIndex;
		}
	}
}
#endif

#if MAX_NUM_SPOT_LIGHTS > 0
void CullSpotLightsPerTile(uint localThreadIndex, float4 viewSpaceFrustumSidePlanes[4], float viewSpaceMinDepth, float viewSpaceMaxDepth)
{
	for (uint lightIndex = localThreadIndex; lightIndex < g_NumSpotLights; lightIndex += NUM_THREADS_PER_TILE)
	{
		Sphere viewSpaceLightBounds = g_SpotLightWorldBoundsBuffer[lightIndex];
		viewSpaceLightBounds.center = mul(g_AppData.viewMatrix, float4(viewSpaceLightBounds.center.xyz, 1.0f)).xyz;

		if (TestSphereAgainstFrustum(viewSpaceFrustumSidePlanes, viewSpaceMinDepth, viewSpaceMaxDepth, viewSpaceLightBounds))
		{
			uint listIndex;
			InterlockedAdd(g_NumSpotLightsPerTile, 1, listIndex);
			g_SpotLightIndicesPerTile[listIndex] = lightIndex;
		}
	}
}
#endif

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void Main(uint3 globalThreadId : SV_DispatchThreadID, uint3 tileId : SV_GroupID, uint localThreadIndex : SV_GroupIndex)
{
	if (localThreadIndex == 0)
	{
		g_ViewSpaceMinDepthIntPerTile = 0x7F7FFFFF;
		g_ViewSpaceMaxDepthIntPerTile = 0;

#if MAX_NUM_POINT_LIGHTS > 0
		g_NumPointLightsPerTile = 0;
		g_PointLightIndicesOffset = 0;
#endif

#if MAX_NUM_SPOT_LIGHTS > 0
		g_NumSpotLightsPerTile = 0;
		g_SpotLightIndicesOffset = 0;
#endif
	}
	GroupMemoryBarrierWithGroupSync();

	float hardwareDepth = g_DepthTexture[globalThreadId.xy].x;
	
	float viewSpaceDepth = ComputeViewSpaceDepth(hardwareDepth, g_AppData.projMatrix);
	uint viewSpaceDepthInt = asuint(viewSpaceDepth);

	InterlockedMin(g_ViewSpaceMinDepthIntPerTile, viewSpaceDepthInt);
	InterlockedMax(g_ViewSpaceMaxDepthIntPerTile, viewSpaceDepthInt);
	GroupMemoryBarrierWithGroupSync();

	float viewSpaceMinDepthPerTile = asfloat(g_ViewSpaceMinDepthIntPerTile);
	float viewSpaceMaxDepthPerTile = asfloat(g_ViewSpaceMaxDepthIntPerTile);

#if (MAX_NUM_POINT_LIGHTS > 0) || (MAX_NUM_SPOT_LIGHTS > 0)
	float4 viewSpaceFrusumSidePlanes[4];
	BuildFrustumSidePlanes(viewSpaceFrusumSidePlanes, tileId.xy);
#endif

	uint tileIndex = tileId.y * NUM_TILES_X + tileId.x;

#if MAX_NUM_POINT_LIGHTS > 0
	CullPointLightsPerTile(localThreadIndex, viewSpaceFrusumSidePlanes, viewSpaceMinDepthPerTile, viewSpaceMaxDepthPerTile);
	GroupMemoryBarrierWithGroupSync();

	if (localThreadIndex == 0)
	{
		uint lightIndicesOffset = 0;
		if (g_NumPointLightsPerTile > 0)
		{
			InterlockedAdd(g_PointLightIndicesOffsetBuffer[0], g_NumPointLightsPerTile, lightIndicesOffset);
			g_PointLightIndicesOffset = lightIndicesOffset;
		}
		g_PointLightRangePerTileBuffer[tileIndex].start = lightIndicesOffset;
		g_PointLightRangePerTileBuffer[tileIndex].length = g_NumPointLightsPerTile;
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localThreadIndex; index < g_NumPointLightsPerTile; index += NUM_THREADS_PER_TILE)
		g_PointLightIndexPerTileBuffer[g_PointLightIndicesOffset + index] = g_PointLightIndicesPerTile[index];
#endif
	
#if MAX_NUM_SPOT_LIGHTS > 0
	CullSpotLightsPerTile(localThreadIndex, viewSpaceFrusumSidePlanes, viewSpaceMinDepthPerTile, viewSpaceMaxDepthPerTile);
	GroupMemoryBarrierWithGroupSync();

	if (localThreadIndex == 0)
	{
		uint lightIndicesOffset = 0;
		if (g_NumSpotLightsPerTile > 0)
		{
			InterlockedAdd(g_SpotLightIndicesOffsetBuffer[0], g_NumSpotLightsPerTile, lightIndicesOffset);
			g_SpotLightIndicesOffset = lightIndicesOffset;
		}
		g_SpotLightRangePerTileBuffer[tileIndex].start = lightIndicesOffset;
		g_SpotLightRangePerTileBuffer[tileIndex].length = g_NumSpotLightsPerTile;
	}
	GroupMemoryBarrierWithGroupSync();

	for (uint index = localThreadIndex; index < g_NumSpotLightsPerTile; index += NUM_THREADS_PER_TILE)
		g_SpotLightIndexPerTileBuffer[g_SpotLightIndicesOffset + index] = g_SpotLightIndicesPerTile[index];
#endif
}
