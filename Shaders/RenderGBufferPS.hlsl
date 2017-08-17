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

	return output;
}