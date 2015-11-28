struct PSInput
{
	float4 screenSpacePos	: SV_Position;
	float2 texCoord			: TEXCOORD0;
};

Texture2D SourceTexture	: register(t0);
SamplerState Sampler : register(s0);

float CalcLuminance(float3 color)
{
	return dot(float3(0.299f, 0.587f, 0.114f), color);
}

float4 Main(PSInput input) : SV_Target
{
	float3 color = SourceTexture.Sample(Sampler, input.texCoord).rgb;
	float luminance = max(CalcLuminance(color), 0.0001f);

#if LOG_LUMINANCE == 1
	luminance = log(luminance);
#endif

	return float4(luminance, 1.0f, 1.0f, 1.0f);
}