#include "Reconstruction.hlsl"
#include "Foundation.hlsl"
#include "GammaCorrection.hlsl"

#define TEXTURE_TYPE_GBUFFER_NORMAL				1
#define TEXTURE_TYPE_GBUFFER_TEXCOORD			2
#define TEXTURE_TYPE_DEPTH						3
#define TEXTURE_TYPE_EXP_SHADOW_MAP				4
#define TEXTURE_TYPE_RGB						5
#define TEXTURE_TYPE_R							6

struct PSInput
{
	float4 screenSpacePos	: SV_Position;
	float2 texCoord			: TEXCOORD0;
};

cbuffer AppDataBuffer : register(b0)
{
	AppData g_AppData;
};

Texture2D g_Texture		: register(t0);
SamplerState g_Sampler	: register(s0);

float4 Main(PSInput input) : SV_Target
{
#if (TEXTURE_TYPE == TEXTURE_TYPE_GBUFFER_NORMAL)
	float3 worldSpaceNormal = g_Texture.Sample(g_Sampler, input.texCoord).rgb;
	float4 color = float4(0.5f * worldSpaceNormal + 0.5f, 1.0f);
#endif // TEXTURE_TYPE_GBUFFER_NORMAL

#if (TEXTURE_TYPE == TEXTURE_TYPE_GBUFFER_TEXCOORD)
	float2 texCoord = g_Texture.Sample(g_Sampler, input.texCoord).rgb;
	float4 color = float4(texCoord, 0.0f, 1.0f);
#endif // TEXTURE_TYPE_GBUFFER_TEXCOORD

#if (TEXTURE_TYPE == TEXTURE_TYPE_DEPTH)
	float hardwareDepth = g_Texture.Sample(g_Sampler, input.texCoord).r;
	float viewSpaceDepth = ComputeViewSpaceDepth(hardwareDepth, g_AppData.projMatrix);
	float normalizedViewSpaceDepth = NormalizeViewSpaceDepth(viewSpaceDepth, g_AppData.cameraNearPlane, g_AppData.cameraFarPlane);
	float4 color = float4(normalizedViewSpaceDepth.rrr, 1.0f);
#endif // TEXTURE_TYPE_DEPTH

#if (TEXTURE_TYPE == TEXTURE_TYPE_EXP_SHADOW_MAP)
	float normalizedLightSpaceDepth = g_Texture.Sample(g_Sampler, input.texCoord).r;
	float4 color = float4(normalizedLightSpaceDepth.rrr, 1.0f);
#endif // TEXTURE_TYPE_EXP_SHADOW_MAP

#if (TEXTURE_TYPE == TEXTURE_TYPE_RGB)
	float3 value = g_Texture.Sample(g_Sampler, input.texCoord).rgb;
	float4 color = float4(value.rgb, 1.0f);
#endif // TEXTURE_TYPE_RGB

#if (TEXTURE_TYPE == TEXTURE_TYPE_R)
	float value = g_Texture.Sample(g_Sampler, input.texCoord).r;
	float4 color = float4(value.rrr, 1.0f);
#endif // TEXTURE_TYPE_R

	return color;
}