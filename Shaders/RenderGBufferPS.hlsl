#include "Foundation.hlsl"

struct PSInput
{
	float4 screenSpacePos		: SV_Position;
	float3 worldSpaceNormal		: NORMAL;
	float2 texCoord				: TEXCOORD;
};

struct PSOutput
{
	float2 texCoord				: SV_Target0;
	float4 worldSpaceNormal		: SV_Target1;
	uint materialIndex			: SV_Target2;
};

cbuffer MaterialIndexBuffer : register(b0)
{
	uint g_MaterialIndex;
}

PSOutput Main(PSInput input)
{
	float3 worldSpaceNormal = normalize(input.worldSpaceNormal);

	PSOutput output;
	output.texCoord = frac(input.texCoord);
	output.worldSpaceNormal = float4(worldSpaceNormal, 0.0f);
	output.materialIndex = g_MaterialIndex;

	return output;
}