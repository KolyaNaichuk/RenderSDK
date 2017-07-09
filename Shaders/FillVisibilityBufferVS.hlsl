struct VSOutput
{
	uint occludeeIndex : OCCLUDEE_INDEX;
	float4 clipSpacePos : SV_Position;
};

struct CameraData
{
	matrix viewProjMatrix;
};

cbuffer CameraDataBuffer : register(b0)
{
	CameraData g_CameraData;
}

StructuredBuffer<uint> g_InstanceIndexBuffer : register(t0);
StructuredBuffer<matrix> g_InstanceWorldMatrixBuffer : register(t1); // Kolya. Potential issue with matrix storage

VSOutput Main(uint instanceId : SV_InstanceID, uint vertexId : SV_VertexID)
{
	uint occludeeIndex = g_InstanceIndexBuffer[instanceId];

	// Kolya. Potential issue with matrix storage and multiplication
	matrix worldMatrix = g_InstanceWorldMatrixBuffer[occludeeIndex];
	matrix worldViewProjMatrix = mul(worldMatrix, g_CameraData.viewProjMatrix);
	
	float4 localSpacePos = float4(
		((vertexId & 0x4) == 0) ? -1.0f : 1.0f,
		((vertexId & 0x2) == 0) ? -1.0f : 1.0f,
		((vertexId & 0x1) == 0) ? -1.0f : 1.0f,
		1.0f);

	VSOutput output;
	output.occludeeIndex = occludeeIndex;
	output.clipSpacePos = mul(localSpacePos, worldViewProjMatrix);

	// Kolya. No need for false-negative pass
	float viewSpaceDepth = output.clipSpacePos.w;
	if (viewSpaceDepth < 0.0f)
		output.clipSpacePos = float4(clamp(output.clipSpacePos.xy, float2(-0.999f, -0.999f), float2(0.999f, 0.999f)), 0.0001f, 1.0f);

	return output;
}