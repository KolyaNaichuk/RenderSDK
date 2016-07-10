#include "Lighting.hlsl"

#define LIGHT_TYPE_POINT	1
#define LIGHT_TYPE_SPOT		2

struct GSInput
{
	float4 worldSpacePos		: SV_Position;
	uint lightIndex				: LIGHT_INDEX;
};

struct GSOutput
{
	float4 clipSpacePos			: SV_Position;
	float4 tileClipDist			: SV_ClipDistance;
};

struct Frustum
{
	float4 leftPlane;
	float4 rightPlane;
	float4 topPlane;
	float4 bottomPlane;
};

#if LIGHT_TYPE == LIGHT_TYPE_POINT

[maxvertexcount(3)]
void Main(triangle GSInput input[3], inout TriangleStream<GSOutput> outputStream)
{
	const uint lightIndex = input[0].lightIndex;
}

#endif // #if LIGHT_TYPE == LIGHT_TYPE_POINT

#if LIGHT_TYPE == LIGHT_TYPE_SPOT
StructuredBuffer<SpotLightProps> g_SpotLightPropsBuffer : register(t0);
StructuredBuffer<matrix> g_SpotLightViewTileProjMatrixBuffer : register(t1);
StructuredBuffer<Frustum> g_SpotLightWorldSpaceFrustumBuffer : register(t2);

[maxvertexcount(3)]
void Main(triangle GSInput input[3], inout TriangleStream<GSOutput> outputStream)
{
	const uint lightIndex = input[0].lightIndex;
	float3 worldSpaceLightDir = g_SpotLightPropsBuffer[lightIndex].worldSpaceDir;

	float3 worldSpaceFaceSide1 = input[1].worldSpacePos.xyz - input[0].worldSpacePos.xyz;
	float3 worldSpaceFaceSide2 = input[2].worldSpacePos.xyz - input[0].worldSpacePos.xyz;
	float3 worldSpaceFaceNormal = normalize(cross(worldSpaceFaceSide1, worldSpaceFaceSide2));
	
	bool isBackFace = (dot(worldSpaceFaceNormal, worldSpaceLightDir) > 0.0f);
	if (isBackFace)
		return;

	Frustum lightWorldSpaceFrustum = g_SpotLightWorldSpaceFrustumBuffer[lightIndex];
	
	float4 tileClipSignedDist[3];
	for (uint vertexIndex = 0; vertexIndex < 3; ++vertexIndex)
	{
		tileClipSignedDist[vertexIndex].x = dot(lightWorldSpaceFrustum.leftPlane, input[vertexIndex].worldSpacePos);
		tileClipSignedDist[vertexIndex].y = dot(lightWorldSpaceFrustum.rightPlane, input[vertexIndex].worldSpacePos);
		tileClipSignedDist[vertexIndex].z = dot(lightWorldSpaceFrustum.topPlane, input[vertexIndex].worldSpacePos);
		tileClipSignedDist[vertexIndex].w = dot(lightWorldSpaceFrustum.bottomPlane, input[vertexIndex].worldSpacePos);
	}
	
	bool isFaceInvisible = (all(tileClipSignedDist[0] < 0.0f) && all(tileClipSignedDist[1] < 0.0f) && all(tileClipSignedDist[2] < 0.0f));
	if (isFaceInvisible)
		return;

	matrix lightViewTileProjMatrix = g_SpotLightViewTileProjMatrixBuffer[lightIndex];
	for (uint vertexIndex = 0; vertexIndex < 3; ++vertexIndex)
	{
		GSOutput output;
		
		output.clipSpacePos = mul(input[vertexIndex].worldSpacePos, lightViewTileProjMatrix);
		output.tileClipDist = tileClipSignedDist[vertexIndex];

		outputStream.Append(output);
	}
	outputStream.RestartStrip();
}

#endif // #if LIGHT_TYPE == LIGHT_TYPE_SPOT