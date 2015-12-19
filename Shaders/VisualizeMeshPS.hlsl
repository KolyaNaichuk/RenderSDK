struct PSInput
{
	float4 screenSpacePos		: SV_Position;

#ifdef HAS_NORMAL
	float3 worldSpaceNormal		: NORMAL;
#endif // HAS_NORMAL

#ifdef HAS_COLOR
	float4 color				: COLOR;
#endif // HAS_COLOR

#ifdef HAS_TEXCOORD
	float2 texCoord				: TEXCOORD;
#endif // HAS_TEXCOORD
};

float4 Main(PSInput input) : SV_Target
{
#ifdef HAS_NORMAL
	float3 normal = normalize(input.worldSpaceNormal);
	float4 color = float4(0.5f * normal + 0.5f, 1.0f);
	return color;
#endif // HAS_NORMAL

#ifdef HAS_COLOR
	return input.color;
#endif // HAS_COLOR

#ifdef HAS_TEXCOORD
	float4 color = float4(frac(input.texCoord), 0.0f, 1.0f);
	if (any(saturate(input.texCoord) - input.texCoord))
		color.b = 0.5f;
	return color;
#endif // HAS_TEXCOORD
}