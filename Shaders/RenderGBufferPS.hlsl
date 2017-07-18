#include "Foundation.hlsl"

struct PSInput
{
	uint   materialIndex		: MATERIAL_INDEX;
	float4 screenSpacePos		: SV_Position;
	float3 worldSpaceNormal		: NORMAL;
	float2 texCoord				: TEXCOORD;
};

// Kolya. Missing implementation

struct PSOutput
{
};

PSOutput Main(PSInput input)
{
	PSOutput output;
	return output;
}