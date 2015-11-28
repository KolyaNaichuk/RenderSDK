struct VSInput
{
	uint vertexId			: SV_VertexID;
};

struct VSOutput
{
	float4 clipSpacePos		: SV_Position;
	float2 texCoord			: TEXCOORD0;
};

VSOutput Main(VSInput input)
{
	VSOutput output;
	
	output.texCoord = float2((input.vertexId << 1) & 2, input.vertexId & 2);
	output.clipSpacePos = float4(output.texCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
	
	return output;
} 

