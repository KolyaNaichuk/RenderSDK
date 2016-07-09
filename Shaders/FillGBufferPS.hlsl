#include "Foundation.hlsl"

struct PSInput
{
	float4 screenSpacePos		: SV_Position;
	float3 worldSpaceNormal		: NORMAL;
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
StructuredBuffer<Material> g_MaterialBuffer : register(t0);

PSOutput Main(PSInput input)
{
	PSOutput output;

	output.worldSpaceNormal = float4(normalize(input.worldSpaceNormal), 0.0f);
	output.diffuseColor = float4(g_MaterialBuffer[g_MaterialIndex].diffuseColor.rgb, 1.0f);
	output.specularColor = float4(g_MaterialBuffer[g_MaterialIndex].specularColor.rgb, g_MaterialBuffer[g_MaterialIndex].specularPower);

	return output;
}