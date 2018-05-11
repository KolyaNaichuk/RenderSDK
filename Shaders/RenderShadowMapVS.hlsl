struct VSInput
{
	uint   instanceId			: SV_InstanceID;
	float3 localSpacePos		: POSITION;
	float3 localSpaceNormal		: NORMAL;
	float2 texCoord				: TEXCOORD;
};

cbuffer Constants32BitBuffer : register(b0)
{
	uint g_InstanceOffset;
}

Buffer<uint> g_MeshInstanceIndexBuffer : register(t0);
StructuredBuffer<float4x4> g_MeshInstanceWorldMatrixBuffer : register(t1);
StructuredBuffer<float4x4> g_SpotLightViewProjMatrixBuffer : register(t2);

float4 Main(VSInput input) : SV_Position
{
	uint instanceIndex = g_MeshInstanceIndexBuffer[g_InstanceOffset + input.instanceId];

	float4x4 worldMatrix = g_MeshInstanceWorldMatrixBuffer[instanceIndex];
	float4x4 viewProjMatrix = g_SpotLightViewProjMatrixBuffer[g_SpotLightIndex];

	float4 worldSpacePos = mul(worldMatrix, float4(input.localSpacePos, 1.0f));
	float4 clipSpacePos = mul(viewProjMatrix, worldSpacePos);

	return clipSpacePos;
}