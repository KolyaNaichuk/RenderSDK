#include "VoxelGrid.hlsl"

struct VSInput
{
	float3 localSpacePos		: POSITION;
	float3 localSpaceNormal		: NORMAL;

#ifdef HAS_TEXCOORD
	float2 texCoord				: TEXCOORD;
#else // HAS_COLOR
	float4 color				: COLOR;
#endif // HAS_TEXCOORD
};

struct VSOutput
{
	float4 worldSpacePos		: SV_Position;
	float4 worldSpaceNormal		: NORMAL;

#ifdef HAS_TEXCOORD
	float2 texCoord				: TEXCOORD;
#else // HAS_COLOR
	float4 color				: COLOR;
#endif // HAS_TEXCOORD
};

cbuffer TransformBuffer : register(b0)
{
	ObjectTransform g_Transform;
}

VSOutput Main(VSInput input)
{
	VSOutput output;
	output.worldSpacePos = mul(float4(input.localSpacePos.xyz, 1.0f), g_Transform.worldPosMatrix);
	output.worldSpaceNormal = mul(float4(input.localSpaceNormal.xyz, 0.0f), g_Transform.worldNormalMatrix);
	
#ifdef HAS_TEXCOORD
	output.texCoord = input.texCoord;
#else // HAS_COLOR
	output.color = input.color;
#endif // HAS_TEXCOORD
	
	return output;
}