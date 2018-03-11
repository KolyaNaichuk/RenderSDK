struct PSInput
{
	float4 screenSpacePos	: SV_Position;
	float2 texCoord			: TEXCOORD0;
};

cbuffer NumMeshTypesBuffer : register(b0)
{
	uint g_NumMeshTypes;
}

Texture2D<uint2> g_GBuffer3 : register(t0);
Buffer<uint> g_MeshTypePerMaterialIDBuffer : register(t1);

float Main(PSInput input) : SV_Depth
{
	uint2 texturePos = uint2(input.screenSpacePos.xy);

	uint materialID = g_GBuffer3[texturePos].g;
	uint meshType = g_MeshTypePerMaterialIDBuffer[materialID];

	return float(meshType) / float(g_NumMeshTypes);
}
