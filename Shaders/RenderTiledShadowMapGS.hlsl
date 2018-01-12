#include "Lighting.hlsl"
#include "OverlapTest.hlsl"

#define LIGHT_TYPE_POINT		1
#define LIGHT_TYPE_SPOT			2

#define NUM_VERTICES			3
#define NUM_CUBE_FACES			6

struct GSInput
{
	float4 worldSpacePos				: SV_Position;
	uint   lightIndex					: LIGHT_INDEX;
};

struct GSOutput
{
	float4 clipSpacePos					: SV_Position;
	float4 signedDistToFrustumPlanes	: SV_ClipDistance;
};

struct Frustum
{
	float4 leftPlane;
	float4 rightPlane;
	float4 topPlane;
	float4 bottomPlane;
};

#if LIGHT_TYPE == LIGHT_TYPE_POINT
StructuredBuffer<Sphere> g_PointLightWorldBoundsBuffer : register(t0);
StructuredBuffer<Frustum> g_PointLightWorldFrustumBuffer : register(t1);
StructuredBuffer<float4x4> g_PointLightViewProjMatrixBuffer : register(t2);

[maxvertexcount(NUM_CUBE_FACES * NUM_VERTICES)]
void Main(triangle GSInput input[NUM_VERTICES], inout TriangleStream<GSOutput> outputStream)
{
	uint lightIndex = input[0].lightIndex;

	float3 faceWorldSpaceEdge1 = input[1].worldSpacePos.xyz - input[0].worldSpacePos.xyz;
	float3 faceWorldSpaceEdge2 = input[2].worldSpacePos.xyz - input[0].worldSpacePos.xyz;
	float3 faceWorldSpaceNormal = cross(faceWorldSpaceEdge1, faceWorldSpaceEdge2);

	float3 lightWorldSpacePos = g_PointLightWorldBoundsBuffer[lightIndex].center;
	float3 lightWorldSpaceDir = input[0].worldSpacePos.xyz - lightWorldSpacePos;
	bool isFacingAwayFromLight = dot(faceWorldSpaceNormal, lightWorldSpaceDir) > 0.0f;
	if (isFacingAwayFromLight)
		return;

	for (uint faceIndex = 0; faceIndex < NUM_CUBE_FACES; ++faceIndex)
	{
		uint frustumIndex = NUM_CUBE_FACES * lightIndex + faceIndex;
		Frustum lightWorldFrustum = g_PointLightWorldFrustumBuffer[frustumIndex];

		float4 signedDistToFrustumPlanes[NUM_VERTICES];
		for (uint vertexIndex = 0; vertexIndex < NUM_VERTICES; ++vertexIndex)
		{
			float4 worldSpacePos = input[vertexIndex].worldSpacePos;

			signedDistToFrustumPlanes[index].x = dot(lightWorldFrustum.leftPlane, worldSpacePos);
			signedDistToFrustumPlanes[index].y = dot(lightWorldFrustum.rightPlane, worldSpacePos);
			signedDistToFrustumPlanes[index].z = dot(lightWorldFrustum.topPlane, worldSpacePos);
			signedDistToFrustumPlanes[index].w = dot(lightWorldFrustum.bottomPlane, worldSpacePos);
		}

		bool isFaceInFrustum = all(signedDistToFrustumPlanes[0] > 0.0f) || all(signedDistToFrustumPlanes[1] > 0.0f) || all(signedDistToFrustumPlanes[2] > 0.0f);
		if (isFaceInFrustum)
		{
			float4x4 lightViewProjMatrix = g_PointLightViewProjMatrixBuffer[frustumIndex];
			for (int vertexIndex = 0; vertexIndex < NUM_VERTICES; ++vertexIndex)
			{
				GSOutput output;
				output.clipSpacePos = mul(lightViewProjMatrix, input[vertexIndex].worldSpacePos);
				output.signedDistToFrustumPlanes = signedDistToFrustumPlanes[vertexIndex];

				outputStream.Append(output);
			}
			outputStream.RestartStrip();
		}
	}
}
#endif // LIGHT_TYPE == LIGHT_TYPE_POINT

#if LIGHT_TYPE == LIGHT_TYPE_SPOT
StructuredBuffer<SpotLightProps> g_SpotLightPropsBuffer : register(t3);
StructuredBuffer<Frustum> g_SpotLightWorldFrustumBuffer : register(t4);
StructuredBuffer<float4x4> g_SpotLightViewProjMatrixBuffer : register(t5);

[maxvertexcount(NUM_VERTICES)]
void Main(triangle GSInput input[NUM_VERTICES], inout TriangleStream<GSOutput> outputStream)
{
	uint lightIndex = input[0].lightIndex;

	float3 faceWorldSpaceEdge1 = input[1].worldSpacePos.xyz - input[0].worldSpacePos.xyz;
	float3 faceWorldSpaceEdge2 = input[2].worldSpacePos.xyz - input[0].worldSpacePos.xyz;
	float3 faceWorldSpaceNormal = cross(faceWorldSpaceEdge1, faceWorldSpaceEdge2);
	
	float3 lightWorldSpaceDir = g_SpotLightPropsBuffer[lightIndex].worldSpaceDir;
	bool isFacingAwayFromLight = dot(faceWorldSpaceNormal, lightWorldSpaceDir) > 0.0f;
	if (isFacingAwayFromLight)
		return;

	Frustum lightWorldFrustum = g_SpotLightWorldFrustumBuffer[lightIndex];

	float4 signedDistToFrustumPlanes[NUM_VERTICES];
	for (uint vertexIndex = 0; vertexIndex < NUM_VERTICES; ++vertexIndex)
	{
		float4 worldSpacePos = input[vertexIndex].worldSpacePos;
				
		signedDistToFrustumPlanes[index].x = dot(lightWorldFrustum.leftPlane, worldSpacePos);
		signedDistToFrustumPlanes[index].y = dot(lightWorldFrustum.rightPlane, worldSpacePos);
		signedDistToFrustumPlanes[index].z = dot(lightWorldFrustum.topPlane, worldSpacePos);
		signedDistToFrustumPlanes[index].w = dot(lightWorldFrustum.bottomPlane, worldSpacePos);
	}

	bool isFaceInFrustum = all(signedDistToFrustumPlanes[0] > 0.0f) || all(signedDistToFrustumPlanes[1] > 0.0f) || all(signedDistToFrustumPlanes[2] > 0.0f);
	if (isFaceInFrustum)
	{
		float4x4 lightViewProjMatrix = g_SpotLightViewProjMatrixBuffer[lightIndex];
		for (int vertexIndex = 0; vertexIndex < NUM_VERTICES; ++vertexIndex)
		{
			GSOutput output;
			output.clipSpacePos = mul(lightViewProjMatrix, input[vertexIndex].worldSpacePos);
			output.signedDistToFrustumPlanes = signedDistToFrustumPlanes[vertexIndex];

			outputStream.Append(output);
		}
		outputStream.RestartStrip();
	}
}
#endif // LIGHT_TYPE == LIGHT_TYPE_SPOT