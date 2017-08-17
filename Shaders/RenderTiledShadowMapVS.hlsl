struct VSInput
{
	uint instanceId				: SV_InstanceID;
	float3 localSpacePos		: POSITION;
	float3 localSpaceNormal		: NORMAL;
};

struct VSOutput
{
	float4 worldSpacePos		: SV_Position;
	uint lightIndexOffset		: LIGHT_INDEX_OFFSET;
};

struct ObjectTransform
{
	float4x4 worldPositionMatrix;
	float4x4 worldNormalMatrix;
	float4x4 worldViewProjMatrix;
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

	output.worldSpacePos = mul(g_Transform.worldPositionMatrix, float4(input.localSpacePos.xyz, 1.0f));
	output.lightIndexOffset = g_LightIndexOffset + input.instanceId;

	return output;
}