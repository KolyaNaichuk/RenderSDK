#include "GammaCorrection.hlsl"

#define TEXTURE_TYPE_NORMAL						1
#define TEXTURE_TYPE_TEXCOORD					2
#define TEXTURE_TYPE_RGB						3
#define TEXTURE_TYPE_R							4

struct PSInput
{
	float4 screenSpacePos	: SV_Position;
	float2 texCoord			: TEXCOORD0;
};

Texture2D g_Texture		: register(t0);
SamplerState g_Sampler	: register(s0);

float4 Main(PSInput input) : SV_Target
{
#if (TEXTURE_TYPE == TEXTURE_TYPE_NORMAL)
	float3 worldSpaceNormal = g_Texture.Sample(g_Sampler, input.texCoord).rgb;
	float3 color = 0.5f * worldSpaceNormal + 0.5f;
#endif // TEXTURE_TYPE_NORMAL

#if (TEXTURE_TYPE == TEXTURE_TYPE_TEXCOORD)
	float2 texCoord = g_Texture.Sample(g_Sampler, input.texCoord).rg;
	float3 color = float3(texCoord, 0.0f);
#endif // TEXTURE_TYPE_TEXCOORD

#if (TEXTURE_TYPE == TEXTURE_TYPE_RGB)
	float3 color = g_Texture.Sample(g_Sampler, input.texCoord).rgb;
#endif // TEXTURE_TYPE_RGB

#if (TEXTURE_TYPE == TEXTURE_TYPE_R)
	float3 color = g_Texture.Sample(g_Sampler, input.texCoord).rrr;
#endif // TEXTURE_TYPE_R

	return float4(GammaCorrection(color), 1.0f);
}