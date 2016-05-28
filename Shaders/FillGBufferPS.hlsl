struct PSInput
{
	float4 screenSpacePos		: SV_Position;
	float3 worldSpaceNormal		: NORMAL;
};

struct PSOutput
{
	float3 worldSpaceNormal		: SV_Target0;
	float4 diffuseColor			: SV_Target1;
	float4 specularColor		: SV_Target2;
};

struct Material
{
	float4 ambientColor;
	float4 diffuseColor;
	float4 specularColor;
	float  specularPower;
	float4 emissiveColor;
};

cbuffer MaterialIndexBuffer : register(b0)
{
	uint g_MaterialIndex;
}
StructuredBuffer<Material> g_MaterialBuffer : register(t0);

PSOutput Main(PSInput input)
{
	PSOutput output;

	output.worldSpaceNormal = normalize(input.worldSpaceNormal);
	output.diffuseColor = float4(g_MaterialBuffer[g_MaterialIndex].diffuseColor.rgb, 1.0f);
	output.specularColor = float4(g_MaterialBuffer[g_MaterialIndex].specularColor.rgb, g_MaterialBuffer[g_MaterialIndex].specularPower);

	return output;
}