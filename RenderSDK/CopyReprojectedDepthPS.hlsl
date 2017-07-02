struct PSInput
{
	float4 screenSpacePos	: SV_Position;
	float2 texCoord			: TEXCOORD0;
};

Texture2D<uint> g_ReprojectedDepthTexture : register(t0);

float Main(PSInput input) : SV_Depth
{
	int2 texturePos = int2(input.screenSpacePos.xy);

	float invDepthInt = g_ReprojectedDepthTexture[texturePos].r;
	float invDepth = asfloat(invDepthInt);
	float depth = 1.0f - invDepth;

	return depth;
}