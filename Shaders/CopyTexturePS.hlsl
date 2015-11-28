struct PSInput
{
	float4 screenSpacePos	: SV_Position;
	float2 texCoord			: TEXCOORD0;
};

Texture2D SourceTexture		: register(t0);
SamplerState Sampler		: register(s0);

float4 Main(PSInput input)	: SV_Target
{
	return SourceTexture.Sample(Sampler, input.texCoord).rgba;
}