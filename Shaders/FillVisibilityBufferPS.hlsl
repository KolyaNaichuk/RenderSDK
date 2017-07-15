struct PSInput
{
	uint instanceIndex		: INSTANCE_INDEX;
	float4 screenSpacePos	: SV_Position;
};

RWStructuredBuffer<uint> g_VisibilityBuffer : register(u0);

[earlydepthstencil]
void Main(PSInput input)
{
	g_VisibilityBuffer[input.instanceIndex] = 1;
}