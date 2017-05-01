struct PSInput
{
	float4 screenSpacePos : SV_Position;
	float4 color : COLOR;
};

float4 Main(PSInput input) : SV_Target
{
	return input.color;
}