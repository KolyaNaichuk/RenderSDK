#include "Foundation.hlsl"

struct VSInput
{
	uint instanceId		: SV_InstanceID;
	uint vertexId		: SV_VertexID;
};

struct VSOutput
{
	uint instanceIndex	: INSTANCE_INDEX;
	float4 clipSpacePos : SV_Position;
};

cbuffer AppDataBuffer : register(b0)
{
	AppData g_AppData;
}

Buffer<uint> g_InstanceIndexBuffer : register(t0);
StructuredBuffer<matrix> g_InstanceWorldMatrixBuffer : register(t1);

VSOutput Main(VSInput input)
{
	uint instanceIndex = g_InstanceIndexBuffer[input.instanceId];
	
	float4 localSpacePos = float4(
		((input.vertexId & 1) == 0) ? -1.0f : 1.0f,
		((input.vertexId & 2) == 0) ? -1.0f : 1.0f,
		((input.vertexId & 4) == 0) ? -1.0f : 1.0f,
		1.0f);

	matrix worldMatrix = g_InstanceWorldMatrixBuffer[instanceIndex];
	float4 worldSpacePos = mul(worldMatrix, localSpacePos);

	VSOutput output;
	output.instanceIndex = instanceIndex;
	output.clipSpacePos = mul(g_AppData.viewProjMatrix, worldSpacePos);

#if CLAMP_VERTICES_BEHIND_CAMERA_NEAR_PLANE == 1
	float viewSpaceDepth = output.clipSpacePos.w;
	if (viewSpaceDepth < 0.0f)
		output.clipSpacePos = float4(clamp(output.clipSpacePos.xy, float2(-0.999f, -0.999f), float2(0.999f, 0.999f)), 0.0001f, 1.0f);
#endif // CLAMP_VERTICES_BEHIND_CAMERA_NEAR_PLANE

	return output;
}