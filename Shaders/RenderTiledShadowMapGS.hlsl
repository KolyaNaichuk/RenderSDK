#include "Lighting.hlsl"
#include "OverlapTest.hlsl"

#define NUM_VERTICES	3

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

#if ENABLE_SPOT_LIGHTS == 1
StructuredBuffer<Frustum> g_SpotLightWorldFrustumBuffer : register(t0);
StructuredBuffer<SpotLightProps> g_SpotLightPropsBuffer : register(t1);
StructuredBuffer<float4x4> g_SpotLightViewProjMatrixBuffer : register(t2);

[maxvertexcount(NUM_VERTICES)]
void Main(triangle GSInput input[NUM_VERTICES], inout TriangleStream<GSOutput> outputStream)
{
	uint lightIndex = input[0].lightIndex;

	float3 faceWorldSpaceEdge1 = input[1].worldSpacePos.xyz - input[0].worldSpacePos.xyz;
	float3 faceWorldSpaceEdge2 = input[2].worldSpacePos.xyz - input[0].worldSpacePos.xyz;
	float3 faceWorldSpaceNormal = cross(faceWorldSpaceEdge1, faceWorldSpaceEdge2);
	
	float3 lightWorldSpaceDir = g_SpotLightPropsBuffer[lightIndex].worldSpaceDir;
	bool isBackFace = (dot(faceWorldSpaceNormal, lightWorldSpaceDir) > 0.0f);
	if (isBackFace)
		return;

	Frustum lightWorldFrustum = g_SpotLightWorldFrustumBuffer[lightIndex];

	uint testFaceAgainstFrustumMask = 0;
	float4 signedDistToFrustumPlanes[NUM_VERTICES];

	for (uint index = 0; index < NUM_VERTICES; ++index)
	{
		float4 worldSpacePos = input[vertexIndex].worldSpacePos;
		
		Probably no need to 
		worldSpacePos /= worldSpacePos.w;
		
		signedDistToFrustumPlanes[index].x = dot(lightWorldFrustum.leftPlane, worldSpacePos);
		signedDistToFrustumPlanes[index].y = dot(lightWorldFrustum.rightPlane, worldSpacePos);
		signedDistToFrustumPlanes[index].z = dot(lightWorldFrustum.topPlane, worldSpacePos);
		signedDistToFrustumPlanes[index].w = dot(lightWorldFrustum.bottomPlane, worldSpacePos);

		testFaceAgainstFrustumMask |= (signedDistToFrustumPlanes[vertexIndex].x > 0) ? 1 : 0;
		testFaceAgainstFrustumMask |= (signedDistToFrustumPlanes[vertexIndex].y > 0) ? 2 : 0;
		testFaceAgainstFrustumMask |= (signedDistToFrustumPlanes[vertexIndex].z > 0) ? 4 : 0;
		testFaceAgainstFrustumMask |= (signedDistToFrustumPlanes[vertexIndex].w > 0) ? 8 : 0;
	}

	if (testFaceAgainstFrustumMask == 15)
	{
		float4x4 lightViewProjMatrix = g_SpotLightViewProjMatrixBuffer[lightIndex];
		for (int index = 0; index < NUM_VERTICES; ++index)
		{
			GSOutput output;
			output.clipSpacePos = mul(lightViewProjMatrix, input[vertexIndex].worldSpacePos);
			output.signedDistToFrustumPlanes = signedDistToFrustumPlanes[index];

			outputStream.Append(output);
		}
		outputStream.RestartStrip();
	}
}
#endif // ENABLE_SPOT_LIGHTS