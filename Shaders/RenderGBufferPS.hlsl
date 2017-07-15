#include "Foundation.hlsl"

struct PSInput
{
	uint   materialId			: MATERIAL_ID;
	float4 screenSpacePos		: SV_Position;
	float3 worldSpaceNormal		: NORMAL;
	float2 texCoord				: TEXCOORD;
};

struct PSOutput
{
};

PSOutput Main(PSInput input)
{
	PSOutput output;
	return output;
}