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
	uint   lightIndex			: LIGHT_INDEX;
};

cbuffer Constants32BitBuffer : register(b0)
{
	uint g_DataOffset;
}

StructuredBuffer<float4x4> g_MeshInstanceWorldMatrixBuffer : register(t0);
Buffer<uint> g_LightIndexForMeshInstanceBuffer : register(t1);
Buffer<uint> g_MeshInstanceIndexForLightBuffer : register(t2);

VSOutput Main(VSInput input)
{
	uint dataOffset = g_DataOffset + input.instanceId;

	uint lightIndex = g_LightIndexForMeshInstanceBuffer[dataOffset];
	uint meshInstanceIndex = g_MeshInstanceIndexForLightBuffer[dataOffset];
	 	
	float4x4 worldMatrix = g_MeshInstanceWorldMatrixBuffer[meshInstanceIndex];

	VSOutput output;
	output.worldSpacePos = mul(worldMatrix, float4(input.localSpacePos, 1.0f));
	output.lightIndex = lightIndex;

	return output;
}