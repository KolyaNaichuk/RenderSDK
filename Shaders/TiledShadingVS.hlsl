struct VSInput
{
	uint vertexId		: SV_VertexID;
};

struct VSOutput
{
	float4 clipSpacePos : SV_Position;
	float2 texCoord		: TEXCOORD0;
};

cbuffer Constants32BitBuffer : register(b0)
{
	uint g_MeshType;
	uint g_NumMeshTypes;
};

VSOutput Main(VSInput input)
{
	VSOutput output;

	float clipSpaceDepth = float(g_MeshType) / float(g_NumMeshTypes);

	output.texCoord = float2((input.vertexId << 1) & 2, input.vertexId & 2);
	output.clipSpacePos = float4(output.texCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), clipSpaceDepth, 1.0f);

	return output;
}