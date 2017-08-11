#include "VoxelGrid.hlsl"

struct VSInput
{
	float3 localSpacePos		: POSITION;
	float3 localSpaceNormal		: NORMAL;

#ifdef HAS_TEXCOORD
	float2 texCoord				: TEXCOORD;
#endif // HAS_TEXCOORD
};

struct VSOutput
{
	float4 worldSpacePos		: SV_Position;
	float4 worldSpaceNormal		: NORMAL;

#ifdef HAS_TEXCOORD
	float2 texCoord				: TEXCOORD;
#endif // HAS_TEXCOORD
};

cbuffer ObjectTransformBuffer : register(b0)
{
	ObjectTransform g_Transform;
}

VSOutput Main(VSInput input)
{
	VSOutput output;
	output.worldSpacePos = mul(g_Transform.worldPositionMatrix, float4(input.localSpacePos.xyz, 1.0f));
	output.worldSpaceNormal = mul(g_Transform.worldNormalMatrix, float4(input.localSpaceNormal.xyz, 0.0f));

#ifdef HAS_TEXCOORD
	output.texCoord = input.texCoord;
#endif // HAS_TEXCOORD
	
	return output;
}