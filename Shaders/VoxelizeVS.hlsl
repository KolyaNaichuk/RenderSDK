struct VSInput
{
	uint   instanceId			: SV_InstanceID;
	float3 localSpacePos		: POSITION;
	float3 localSpaceNormal		: NORMAL;
	float2 texCoord				: TEXCOORD;
};

struct VSOutput
{
	float4 worldSpacePos		: SV_Position;
	float3 worldSpaceNormal		: NORMAL;
	float2 texCoord				: TEXCOORD;
};

cbuffer Constants32BitBuffer : register(b1)
{
	uint g_InstanceOffset;
}

Buffer<uint> g_InstanceIndexBuffer : register(t0);
StructuredBuffer<float4x4> g_InstanceWorldMatrixBuffer : register(t1);

VSOutput Main(VSInput input)
{
	uint instanceIndex = g_InstanceIndexBuffer[g_InstanceOffset + input.instanceId];
	float4x4 worldMatrix = g_InstanceWorldMatrixBuffer[instanceIndex];
	
	VSOutput output;
	output.worldSpacePos = mul(worldMatrix, float4(input.localSpacePos, 1.0f));
	output.worldSpaceNormal = mul(worldMatrix, float4(input.localSpaceNormal, 0.0f)).xyz;
	output.texCoord = input.texCoord;

	return output;
}