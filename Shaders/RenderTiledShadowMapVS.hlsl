struct VSInput
{
	uint instanceId				: SV_InstanceID;
	float3 localSpacePos		: POSITION;
	float3 localSpaceNormal		: NORMAL;
};

struct VSOutput
{
	float4 worldSpacePos		: SV_Position;
	uint lightIndex				: LIGHT_INDEX;
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

cbuffer LightIndexOffsetBuffer : register(b1)
{
	uint g_LightIndexOffset;
}

VSOutput Main(VSInput input)
{
	VSOutput output;

	output.worldSpacePos = mul(float4(input.localSpacePos.xyz, 1.0f), g_Transform.worldPositionMatrix);
	output.lightIndex = g_LightIndexOffset + input.instanceId;

	return output;
}