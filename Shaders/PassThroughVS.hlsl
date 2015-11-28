struct VSInput
{
	float4 clipSpacePos		: POSITION;
	float4 color			: COLOR;
};

struct VSOutput
{
	float4 clipSpacePos		: SV_Position;
	float4 color			: COLOR;
};

VSOutput Main(VSInput input)
{
	VSOutput output;
	output.clipSpacePos = input.clipSpacePos;
	output.color = input.color;
	return output;
}