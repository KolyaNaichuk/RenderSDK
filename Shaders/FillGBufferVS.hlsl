struct VSInput
{
	float3 localSpacePos		: POSITION;
	float3 localSpaceNormal		: NORMAL;
	float4 color				: COLOR;
};

struct VSOutput
{
	float4 clipSpacePos			: SV_Position;
	float3 worldSpaceNormal		: NORMAL;
	float4 color				: COLOR;
};

struct ObjectTransform
{
	matrix worldPosMatrix;
	matrix worldNormalMatrix;
	matrix worldViewProjMatrix;
	float4 notUsed[4];
};

cbuffer TransformBuffer : register(b0)
{
	ObjectTransform g_Transform;
}

VSOutput Main(VSInput input)
{
	VSOutput output;

	output.clipSpacePos = mul(float4(input.localSpacePos.xyz, 1.0f), g_Transform.worldViewProjMatrix);
	output.worldSpaceNormal = mul(float4(input.localSpaceNormal.xyz, 0.0f), g_Transform.worldNormalMatrix).xyz;
	output.color = input.color;

	return output;
}