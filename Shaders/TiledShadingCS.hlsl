#include "Reconstruction.hlsl"
#include "Lighting.hlsl"
#include "OverlapTest.hlsl"

struct ShadingData
{
	float2 rcpScreenSize;
	float2 notUsed1;
	float3 worldSpaceLightDir;
	float  notUsed2;
	float3 directLightColor;
	float  notUsed3;
	float3 worldSpaceCameraPos;
	float  notUsed4;
	matrix viewMatrix;
	matrix projMatrix;
	matrix projInvMatrix;
	matrix viewProjInvMatrix;
	matrix notUsed5[3];
};

#define NUM_THREADS_PER_TILE	(TILE_SIZE * TILE_SIZE)

#if NUM_POINT_LIGHTS > 0
struct PointLightBounds
{
	float3 worldSpaceSphereCenter;
	float sphereRadius;
};

struct PointLightProps
{
	float3 color;
	float attenStartRange;
};
#endif

#if NUM_SPOT_LIGHTS > 0
struct SpotLightBounds
{
	float3 worldSpaceSphereCenter;
	float sphereRadius;
};

struct SpotLightProps
{
	float3 color;
	float3 worldSpaceDir;
	float attenStartRange;
	float attenEndRange;
	float cosHalfInnerConeAngle;
	float cosHalfOuterConeAngle;
};
#endif

float4 CreatePlanePassingThroughOrigin(float3 pt1, float3 pt2)
{
	float3 planeNormal = normalize(cross(pt1, pt2));
	float signedDistFromOrigin = 0.0f;

	return float4(planeNormal, signedDistFromOrigin);
}

void BuildFrustumSidePlanes(out float4 viewSpaceFrusumSidePlanes[4], uint2 tileId, float2 rcpScreenSize, matrix projInvMatrix)
{
	uint2 screenSpaceTileTLCorner = tileId.xy * TILE_SIZE;
	uint2 screenSpaceTileBRCorner = screenSpaceTileTLCorner.xy + TILE_SIZE;

	float2 texSpaceTileTLCorner = (float2(screenSpaceTileTLCorner.xy) + 0.5f) * rcpScreenSize;
	float2 texSpaceTileBRCorner = (float2(screenSpaceTileBRCorner.xy) + 0.5f) * rcpScreenSize;
	float2 texSpaceTileTRCorner =  float2(texSpaceTileBRCorner.x, texSpaceTileTLCorner.y);
	float2 texSpaceTileBLCorner =  float2(texSpaceTileTLCorner.x, texSpaceTileBRCorner.y);

	float3 viewSpaceTileTLCorner = ComputeViewSpacePosition(texSpaceTileTLCorner, 1.0f, projInvMatrix).xyz;
	float3 viewSpaceTileTRCorner = ComputeViewSpacePosition(texSpaceTileTRCorner, 1.0f, projInvMatrix).xyz;
	float3 viewSpaceTileBLCorner = ComputeViewSpacePosition(texSpaceTileBLCorner, 1.0f, projInvMatrix).xyz;
	float3 viewSpaceTileBRCorner = ComputeViewSpacePosition(texSpaceTileBRCorner, 1.0f, projInvMatrix).xyz;

	viewSpaceFrusumSidePlanes[0] = CreatePlanePassingThroughOrigin(viewSpaceTileTRCorner, viewSpaceTileTLCorner);
	viewSpaceFrusumSidePlanes[1] = CreatePlanePassingThroughOrigin(viewSpaceTileBLCorner, viewSpaceTileBRCorner);
	viewSpaceFrusumSidePlanes[2] = CreatePlanePassingThroughOrigin(viewSpaceTileTLCorner, viewSpaceTileBLCorner);
	viewSpaceFrusumSidePlanes[3] = CreatePlanePassingThroughOrigin(viewSpaceTileBRCorner, viewSpaceTileTRCorner);
}

bool TestSphereAgainstFrustum(float4 frustumSidePlanes[4], float frustumMinZ, float frustumMaxZ, float3 sphereCenter, float sphereRadius)
{
	bool insideOrOverlap = TestSphereAgainstPlane(frustumSidePlanes[0], sphereCenter, sphereRadius) &&
		TestSphereAgainstPlane(frustumSidePlanes[1], sphereCenter, sphereRadius) &&
		TestSphereAgainstPlane(frustumSidePlanes[2], sphereCenter, sphereRadius) &&
		TestSphereAgainstPlane(frustumSidePlanes[3], sphereCenter, sphereRadius) &&
		(frustumMinZ < sphereCenter.z + sphereRadius) &&
		(frustumMaxZ > sphereCenter.z - sphereRadius);

	return insideOrOverlap;
}

cbuffer ShadingDataBuffer : register(b0)
{
	ShadingData g_ShadingData;
}

Texture2D g_DepthTexture : register(t0);
Texture2D g_NormalTexture : register(t1);
Texture2D g_DiffuseTexture : register(t2);
Texture2D g_SpecularTexture : register(t3);

#if NUM_POINT_LIGHTS > 0
StructuredBuffer<PointLightBounds> g_PointLightBoundsBuffer : register(t4);
StructuredBuffer<PointLightProps> g_PointLightPropsBuffer : register(t5);
#endif

#if NUM_SPOT_LIGHTS > 0
StructuredBuffer<SpotLightBounds> g_SpotLightBoundsBuffer : register(t6);
StructuredBuffer<SpotLightProps> g_SpotLightPropsBuffer : register(t7);
#endif

RWTexture2D<float4> g_AccumLightTexture : register(u0);

groupshared uint g_ViewSpaceMinDepthIntPerTile;
groupshared uint g_ViewSpaceMaxDepthIntPerTile;

#if NUM_POINT_LIGHTS > 0
groupshared uint g_NumPointLightsPerTile;
groupshared uint g_PointLightIndicesPerTile[NUM_POINT_LIGHTS];
#endif

#if NUM_SPOT_LIGHTS > 0
groupshared uint g_NumSpotLightsPerTile;
groupshared uint g_SpotLightIndicesPerTile[NUM_SPOT_LIGHTS];
#endif

#if NUM_POINT_LIGHTS > 0
void CullPointLightsPerTile(uint localThreadIndex, float4 viewSpaceFrustumSidePlanes[4], float viewSpaceMinDepth, float viewSpaceMaxDepth, matrix viewMatrix)
{
	for (uint lightIndex = localThreadIndex; lightIndex < NUM_POINT_LIGHTS; lightIndex += NUM_THREADS_PER_TILE)
	{
		float3 worldSpaceSphereCenter = g_PointLightBoundsBuffer[lightIndex].worldSpaceSphereCenter;
		float3 viewSpaceSphereCenter = mul(float4(worldSpaceSphereCenter.xyz, 1.0f), viewMatrix).xyz;
		float sphereRadius = g_PointLightBoundsBuffer[lightIndex].sphereRadius;

		if (TestSphereAgainstFrustum(viewSpaceFrustumSidePlanes, viewSpaceMinDepth, viewSpaceMaxDepth, viewSpaceSphereCenter, sphereRadius))
		{
			uint listIndex;
			InterlockedAdd(g_NumPointLightsPerTile, 1, listIndex);
			g_PointLightIndicesPerTile[listIndex] = lightIndex;
		}
	}
}

float3 CalcPointLightsContribution(float3 worldSpaceDirToViewer, float3 worldSpacePos, float3 worldSpaceNormal,
	float3 diffuseAlbedo, float3 specularAlbedo, float specularPower)
{
	float3 lightContrib = float3(0.0f, 0.0f, 0.0f);
	for (uint lightIndexPerTile = 0; lightIndexPerTile < g_NumPointLightsPerTile; ++lightIndexPerTile)
	{
		uint lightIndex = g_PointLightIndicesPerTile[lightIndexPerTile];

		float3 worldSpaceLightPos = g_PointLightBoundsBuffer[lightIndex].worldSpaceSphereCenter;
		float attenEndRange = g_PointLightBoundsBuffer[lightIndex].sphereRadius;
		float attenStartRange = g_PointLightPropsBuffer[lightIndex].attenStartRange;
		float3 lightColor = g_PointLightPropsBuffer[lightIndex].color;

		lightContrib += CalcPointLightContribution(worldSpaceLightPos, lightColor, attenStartRange, attenEndRange,
			worldSpaceDirToViewer, worldSpacePos, worldSpaceNormal, diffuseAlbedo, specularAlbedo, specularPower);
	}
	return lightContrib;
}
#endif

#if NUM_SPOT_LIGHTS > 0
void CullSpotLightsPerTile(uint localThreadIndex, float4 viewSpaceFrustumSidePlanes[4], float viewSpaceMinDepth, float viewSpaceMaxDepth, matrix viewMatrix)
{
	for (uint lightIndex = localThreadIndex; lightIndex < NUM_SPOT_LIGHTS; lightIndex += NUM_THREADS_PER_TILE)
	{
		float3 worldSpaceSphereCenter = g_SpotLightBoundsBuffer[lightIndex].worldSpaceSphereCenter;
		float3 viewSpaceSphereCenter = mul(float4(worldSpaceSphereCenter.xyz, 1.0f), viewMatrix).xyz;
		float sphereRadius = g_SpotLightBoundsBuffer[lightIndex].sphereRadius;

		if (TestSphereAgainstFrustum(viewSpaceFrustumSidePlanes, viewSpaceMinDepth, viewSpaceMaxDepth, viewSpaceSphereCenter, sphereRadius))
		{
			uint listIndex;
			InterlockedAdd(g_NumSpotLightsPerTile, 1, listIndex);
			g_SpotLightIndicesPerTile[listIndex] = lightIndex;
		}
	}
}

float3 CalcSpotLightsContribution(float3 worldSpaceDirToViewer, float3 worldSpacePos, float3 worldSpaceNormal,
	float3 diffuseAlbedo, float3 specularAlbedo, float specularPower)
{
	float3 lightContrib = float3(0.0f, 0.0f, 0.0f);
	for (uint lightIndexPerTile = 0; lightIndexPerTile < g_NumSpotLightsPerTile; ++lightIndexPerTile)
	{
		uint lightIndex = g_SpotLightIndicesPerTile[lightIndexPerTile];
		
		SpotLightBounds lightBounds = g_SpotLightBoundsBuffer[lightIndex];
		float3 worldSpaceLightDir = g_SpotLightPropsBuffer[lightIndex].worldSpaceDir;
		float3 worldSpaceLightPos = lightBounds.worldSpaceSphereCenter - lightBounds.sphereRadius * worldSpaceLightDir;
		float attenEndRange = g_SpotLightPropsBuffer[lightIndex].attenEndRange;
		float attenStartRange = g_SpotLightPropsBuffer[lightIndex].attenStartRange;
		float3 lightColor = g_SpotLightPropsBuffer[lightIndex].color;
		float cosHalfInnerConeAngle = g_SpotLightPropsBuffer[lightIndex].cosHalfInnerConeAngle;
		float cosHalfOuterConeAngle = g_SpotLightPropsBuffer[lightIndex].cosHalfOuterConeAngle;

		lightContrib += CalcSpotLightContribution(worldSpaceLightPos, worldSpaceLightDir, lightColor, attenStartRange, attenEndRange,
			cosHalfInnerConeAngle, cosHalfOuterConeAngle, worldSpaceDirToViewer, worldSpacePos, worldSpaceNormal,
			diffuseAlbedo, specularAlbedo, specularPower);
	}
	return lightContrib;
}
#endif

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void Main(uint3 globalThreadId : SV_DispatchThreadID, uint3 tileId : SV_GroupID, uint localThreadIndex : SV_GroupIndex)
{
	if (localThreadIndex == 0)
	{
		g_ViewSpaceMinDepthIntPerTile = 0x7F7FFFFF;
		g_ViewSpaceMaxDepthIntPerTile = 0;

#if NUM_POINT_LIGHTS > 0
		g_NumPointLightsPerTile = 0;
#endif

#if NUM_SPOT_LIGHTS > 0
		g_NumSpotLightsPerTile = 0;
#endif
	}
	GroupMemoryBarrierWithGroupSync();

	float hardwareDepth = g_DepthTexture[globalThreadId.xy].x;
	float viewSpaceDepth = ComputeViewSpaceDepth(hardwareDepth, g_ShadingData.projMatrix);

	if (hardwareDepth != 1.0f)
	{
		uint viewSpaceDepthInt = asuint(viewSpaceDepth);

		InterlockedMin(g_ViewSpaceMinDepthIntPerTile, viewSpaceDepthInt);
		InterlockedMax(g_ViewSpaceMaxDepthIntPerTile, viewSpaceDepthInt);
	}
	GroupMemoryBarrierWithGroupSync();

	float viewSpaceMinDepthPerTile = asfloat(g_ViewSpaceMinDepthIntPerTile);
	float viewSpaceMaxDepthPerTile = asfloat(g_ViewSpaceMaxDepthIntPerTile);

#if (NUM_POINT_LIGHTS > 0) || (NUM_SPOT_LIGHTS > 0)
	float4 viewSpaceFrusumSidePlanes[4];
	BuildFrustumSidePlanes(viewSpaceFrusumSidePlanes, tileId.xy, g_ShadingData.rcpScreenSize, g_ShadingData.projInvMatrix);
#endif

#if NUM_POINT_LIGHTS > 0
	CullPointLightsPerTile(localThreadIndex, viewSpaceFrusumSidePlanes, viewSpaceMinDepthPerTile, viewSpaceMaxDepthPerTile, g_ShadingData.viewMatrix);
	GroupMemoryBarrierWithGroupSync();
#endif

#if NUM_SPOT_LIGHTS > 0
	CullSpotLightsPerTile(localThreadIndex, viewSpaceFrusumSidePlanes, viewSpaceMinDepthPerTile, viewSpaceMaxDepthPerTile, g_ShadingData.viewMatrix);
	GroupMemoryBarrierWithGroupSync();
#endif

	float2 texCoord = (float2(globalThreadId.xy) + 0.5f) * g_ShadingData.rcpScreenSize;
	float3 worldSpacePos = ComputeWorldSpacePosition(texCoord, hardwareDepth, g_ShadingData.viewProjInvMatrix).xyz;
	float3 worldSpaceDirToViewer = normalize(g_ShadingData.worldSpaceCameraPos - worldSpacePos);
	float3 worldSpaceNormal = g_NormalTexture[globalThreadId.xy].xyz;
	float3 diffuseAlbedo = g_DiffuseTexture[globalThreadId.xy].rgb;
	float4 specularAlbedo = g_SpecularTexture[globalThreadId.xy].rgba;

#if NUM_POINT_LIGHTS > 0
	float3 pointLightsContrib = CalcPointLightsContribution(worldSpaceDirToViewer, worldSpacePos, worldSpaceNormal, diffuseAlbedo, specularAlbedo.rgb, specularAlbedo.a);
#else
	float3 pointLightsContrib = float3(0.0f, 0.0f, 0.0f);
#endif

#if NUM_SPOT_LIGHTS > 0
	float3 spotLightsContrib = CalcSpotLightsContribution(worldSpaceDirToViewer, worldSpacePos, worldSpaceNormal, diffuseAlbedo, specularAlbedo.rgb, specularAlbedo.a);
#else
	float3 spotLightsContrib = float3(0.0f, 0.0f, 0.0f);
#endif

#if USE_DIRECT_LIGHT == 1
	float3 directLightContrib = CalcDirectionalLightContribution(g_ShadingData.worldSpaceLightDir, g_ShadingData.directLightColor, worldSpaceDirToViewer,
		worldSpaceNormal, diffuseAlbedo, specularAlbedo.rgb, specularAlbedo.a);
#else
	float3 directLightContrib = float3(0.0f, 0.0f, 0.0f);
#endif

	float3 accumLight = pointLightsContrib + spotLightsContrib + directLightContrib;
	g_AccumLightTexture[globalThreadId.xy] = float4(accumLight, 1.0f);
}
