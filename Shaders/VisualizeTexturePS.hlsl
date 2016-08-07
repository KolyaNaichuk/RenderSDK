#include "Reconstruction.hlsl"

#define TEXTURE_TYPE_GBUFFER_DIFFUSE	1
#define TEXTURE_TYPE_GBUFFER_SPECULAR	2
#define TEXTURE_TYPE_GBUFFER_NORMAL		3
#define TEXTURE_TYPE_DEPTH				4
#define TEXTURE_TYPE_OTHER				5

struct VisualizeTextureData
{
	matrix cameraProjMatrix;
	float cameraNearPlane;
	float cameraFarPlane;
	float notUsed[46];
};

struct PSInput
{
	float4 screenSpacePos	: SV_Position;
	float2 texCoord			: TEXCOORD0;
};

cbuffer VisualizeTextureDataBuffer : register(b0)
{
	VisualizeTextureData g_VisualizeData;
};

Texture2D InputTexture		: register(t0);
SamplerState Sampler		: register(s0);

float4 Main(PSInput input) : SV_Target
{
#if (TEXTURE_TYPE == TEXTURE_TYPE_GBUFFER_DIFFUSE)
	float3 diffuseColor = InputTexture.Sample(Sampler, input.texCoord).rgb;
	return float4(diffuseColor, 1.0f);
#endif // TEXTURE_TYPE_GBUFFER_DIFFUSE

#if (TEXTURE_TYPE == TEXTURE_TYPE_GBUFFER_SPECULAR)
	float3 specularColor = InputTexture.Sample(Sampler, input.texCoord).rgb;
	return float4(specularColor, 1.0f);
#endif // TEXTURE_TYPE_GBUFFER_SPECULAR

#if (TEXTURE_TYPE == TEXTURE_TYPE_GBUFFER_NORMAL)
	float3 worldSpaceNormal = InputTexture.Sample(Sampler, input.texCoord).rgb;
	return float4(0.5f * worldSpaceNormal + 0.5f, 1.0f);
#endif // TEXTURE_TYPE_GBUFFER_NORMAL

#if (TEXTURE_TYPE == TEXTURE_TYPE_DEPTH)
	float hardwareDepth = InputTexture.Sample(Sampler, input.texCoord).r;
	float viewSpaceDepth = ComputeViewSpaceDepth(hardwareDepth, g_VisualizeData.cameraProjMatrix);
	float normalizedDepth = NormalizeViewSpaceDepth(viewSpaceDepth, g_VisualizeData.cameraNearPlane, g_VisualizeData.cameraFarPlane);
	return float4(normalizedDepth.rrr, 1.0f);
#endif // TEXTURE_TYPE_DEPTH

#if (TEXTURE_TYPE == TEXTURE_TYPE_OTHER)
	return InputTexture.Sample(Sampler, input.texCoord).rgba;
#endif // TEXTURE_TYPE_OTHER
}