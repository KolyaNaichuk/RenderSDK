struct VSInput
{
	float3 localSpacePos		: POSITION;

#ifdef HAS_NORMAL
	float3 localSpaceNormal		: NORMAL;
#endif // HAS_NORMAL

#ifdef HAS_COLOR
	float4 color				: COLOR;
#endif // HAS_COLOR

#ifdef HAS_TANGENT
	float3 tangent				: TANGENT;
#endif // HAS_TANGENT

#ifdef HAS_BITANGENT
	float3 bitangent			: BITANGENT;
#endif // HAS_BITANGENT

#ifdef HAS_TEXCOORD
	float2 texCoord				: TEXCOORD;
#endif // HAS_TEXCOORD
};

struct VSOutput
{
	float4 clipSpacePos			: SV_Position;

#ifdef HAS_NORMAL
	float3 worldSpaceNormal		: NORMAL;
#endif // HAS_NORMAL

#ifdef HAS_COLOR
	float4 color				: COLOR;
#endif // HAS_COLOR

#ifdef HAS_TEXCOORD
	float2 texCoord				: TEXCOORD;
#endif // HAS_TEXCOORD
};

struct ObjectTransform
{
	matrix worldPositionMatrix;
	matrix worldNormalMatrix;
	matrix worldViewProjMatrix;
	float4 notUsed[4];
};

cbuffer ObjectTransformBuffer : register(b0)
{
	ObjectTransform g_Transform;
}

VSOutput Main(VSInput input)
{
	VSOutput output;
	output.clipSpacePos = mul(float4(input.localSpacePos.xyz, 1.0f), g_Transform.worldViewProjMatrix);

#ifdef HAS_NORMAL
	output.worldSpaceNormal = mul(float4(input.localSpaceNormal.xyz, 0.0f), g_Transform.worldNormalMatrix).xyz;
#endif // HAS_NORMAL

#ifdef HAS_COLOR
	output.color = input.color;
#endif // HAS_COLOR

#ifdef HAS_TEXCOORD
	output.texCoord = input.texCoord;
#endif // HAS_TEXCOORD

	return output;
}