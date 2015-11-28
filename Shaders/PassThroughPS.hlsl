struct PSInput
{
	float4 screenSpacePos	: SV_Position;
	float4 color			: COLOR;
};

float4 Main(PSInput input)	: SV_Target
{
#if defined(COLOR_RRRR_COMP)
	return input.color.rrrr;
#elif defined(COLOR_GGGG_COMP)
	return input.color.gggg;
#elif defined(COLOR_BBBB_COMP)
	return input.color.bbbb;
#elif defined(COLOR_ONLY_R_COMP)
	return float4(input.color.r, 0.0, 0.0f, 1.0f);
#elif defined(COLOR_ONLY_G_COMP)
	return float4(0.0f, input.color.g, 0.0f, 1.0f);
#elif defined(COLOR_ONLY_B_COMP)
	return float4(0.0f, 0.0f, input.color.b, 1.0f);
#else
	return input.color.rgba;
#endif
}