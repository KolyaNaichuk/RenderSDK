#include "Foundation.hlsl"

struct PSInput
{
	float4 screenSpacePos		: SV_Position;
	float3 worldSpaceNormal		: NORMAL;

#ifdef HAS_TEXCOORDS
	float2 texCoord				: TEXCOORD;
#endif // HAS_TEXCOORDS
};

struct PSOutput
{
	float4 worldSpaceNormal		: SV_Target0;
	float4 diffuseColor			: SV_Target1;
	float4 specularColor		: SV_Target2;
};

cbuffer MaterialIndexBuffer : register(b0)
{
	uint g_MaterialIndex;
}

StructuredBuffer<MaterialData> g_MaterialDataBuffer : register(t0);
Texture2D g_DiffuseTextures[] : register(t1);
Texture2D g_SpecularTextures[] : register(t2);
Texture2D g_SpecularPowerTextures[] : register(t3);
SamplerState g_Sampler : register(s0);

PSOutput Main(PSInput input)
{
	float3 diffuseColor = g_MaterialDataBuffer[g_MaterialIndex].diffuseColor.rgb;
#ifdef HAS_DIFFUSE_MAP
	Texture2D diffuseTexture = g_DiffuseTextures[g_MaterialIndex];
	diffuseColor *= diffuseTexture.Sample(g_Sampler, input.texCoord).rgb;
#endif // HAS_DIFFUSE_MAP

	float3 specularColor = g_MaterialDataBuffer[g_MaterialIndex].specularColor.rgb;
#ifdef HAS_SPECULAR_MAP
	Texture2D specularTexture = g_SpecularTextures[g_MaterialIndex];
	specularColor *= specularTexture.Sample(g_Sampler, input.texCoord).rgb;
#endif // HAS_SPECULAR_MAP

	float specularPower = g_MaterialDataBuffer[g_MaterialIndex].specularPower;
#ifdef HAS_SPECULAR_POWER_MAP
	Texture2D specularPowerTexture = g_SpecularPowerTextures[g_MaterialIndex];
	specularPower *= specularPowerTexture.Sample(g_Sampler, input.texCoord).r;
#endif // HAS_SPECULAR_POWER_MAP

	PSOutput output;
	output.worldSpaceNormal = float4(normalize(input.worldSpaceNormal), 0.0f);
	output.diffuseColor = float4(diffuseColor, 1.0f);
	output.specularColor = float4(specularColor, specularPower);

	return output;
}