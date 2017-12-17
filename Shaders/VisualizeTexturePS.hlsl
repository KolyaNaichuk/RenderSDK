#include "Reconstruction.hlsl"
#include "Foundation.hlsl"
#include "GammaCorrection.hlsl"

#define TEXTURE_TYPE_GBUFFER_NORMAL		1
#define TEXTURE_TYPE_GBUFFER_TEXCOORD	2
#define TEXTURE_TYPE_DEPTH				3
#define TEXTURE_TYPE_OTHER				4

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
	float normalizedDepth = NormalizeViewSpaceDepth(viewSpaceDepth, g_AppData.cameraNearPlane, g_AppData.cameraFarPlane);
	float4 color = float4(normalizedDepth.rrr, 1.0f);
#endif // TEXTURE_TYPE_DEPTH

#if (TEXTURE_TYPE == TEXTURE_TYPE_OTHER)
	float4 color = g_Texture.Sample(g_Sampler, input.texCoord).rgba;
#endif // TEXTURE_TYPE_OTHER

	return float4(GammaCorrection(color.rgb), color.a);
}