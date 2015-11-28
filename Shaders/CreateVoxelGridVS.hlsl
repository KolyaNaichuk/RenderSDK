#include "VoxelGrid.hlsl"

struct VSInput
{
	float3 localSpacePos		: POSITION;
	float3 localSpaceNormal		: NORMAL;
	float2 texCoord				: TEXCOORD;
};

struct VSOutput
{
	float4 worldSpacePos		: SV_Position;
	float4 worldSpaceNormal		: NORMAL;
	float2 texCoord				: TEXCOORD;
};

cbuffer ObjectTransformBuffer : register(b0)
{
	ObjectTransform g_ObjectTransform;
}

VSOutput Main(VSInput input)
{
	VSOuput output;
	output.worldSpacePos = mul(float4(input.localSpacePos.xyz, 1.0f), g_ObjectTransform.worldMatrix);
	output.worldSpaceNormal = mul(float4(input.localSpaceNormal.xyz, 0.0f), g_ObjectTransform.worldInvTransposeMatrix);
	output.texCoord = input.texCoord;
	return output;
}