#include "Foundation.hlsl"

struct PSInput
{
	float4 screenSpacePos	: SV_Position;
	float2 texCoord			: TEXCOORD0;
};

Texture2D<uint2> g_GBuffer3 : register(t0);
Buffer<uint> g_MeshTypePerMaterialIDBuffer : register(t1);

float Main(PSInput input) : SV_Depth
{
	uint2 pixelPos = uint2(input.screenSpacePos.xy);

	uint materialID = g_GBuffer3[pixelPos].g;
	uint meshType = g_MeshTypePerMaterialIDBuffer[materialID];

	return CalcMeshTypeDepth(meshType);
}
