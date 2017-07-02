struct VSInput
{
	float3 localSpacePos		: POSITION;
	float3 localSpaceNormal		: NORMAL;

#ifdef HAS_TEXCOORDS
	float2 texCoord				: TEXCOORD;
#endif // HAS_TEXCOORDS
};

struct VSOutput
{
	float4 clipSpacePos			: SV_Position;
	float3 worldSpaceNormal		: NORMAL;

#ifdef HAS_TEXCOORDS
	float2 texCoord				: TEXCOORD;
#endif // HAS_TEXCOORDS
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
	output.worldSpaceNormal = mul(float4(input.localSpaceNormal.xyz, 0.0f), g_Transform.worldNormalMatrix).xyz;

#ifdef HAS_TEXCOORDS
	output.texCoord = input.texCoord;
#endif // HAS_TEXCOORDS

	return output;
}