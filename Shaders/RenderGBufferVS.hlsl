struct VSInput
{
	uint   instanceId			: SV_InstanceID
	float3 localSpacePos		: POSITION;
	float3 localSpaceNormal		: NORMAL;
	float2 texCoord				: TEXCOORD;
};

struct VSOutput
{
	uint   materialIndex		: MATERIAL_INDEX;
	float4 clipSpacePos			: SV_Position;
	float3 worldSpaceNormal		: NORMAL;
	float2 texCoord				: TEXCOORD;
};

cbuffer InstanceOffsetBuffer : register(b0)
{
	uint g_InstanceOffset;
	uint g_MaterialIndex;	// Kolya. Check if I could pass it directly to pixel shader
}

StructuredBuffer<uint> g_InstanceIndexBuffer : register(t0);
StructuredBuffer<matrix> g_InstanceWorldMatrixBuffer : register(t1); // Kolya. Potential issue with matrix storage
StructuredBuffer<matrix> g_InstanceWorldViewProjMatrixBuffer : register(t2); // Kolya. Potential issue with matrix storage

VSOutput Main(VSInput input)
{
	uint instanceIndex = g_InstanceIndexBuffer[g_InstanceOffset + input.instanceId];

	matrix worldMatrix = g_InstanceWorldMatrixBuffer[instanceIndex];
	matrix worldViewProjMatrix = g_InstanceWorldViewProjMatrixBuffer[instanceIndex];

	VSOutput output;
	output.materialIndex = g_MaterialIndex;
	output.clipSpacePos = mul(float4(input.localSpacePos, 1.0f), worldViewProjMatrix);
	output.worldSpaceNormal = mul(float4(input.localSpaceNormal, 0.0f), worldMatrix).xyz;
	output.texCoord = input.texCoord;

	return output;
}