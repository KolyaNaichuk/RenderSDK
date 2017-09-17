struct PSInput
{
	float4 screenSpacePos	: SV_Position;
	float2 texCoord			: TEXCOORD0;
};

cbuffer NumMeshTypesBuffer : register(b0)
{
	uint g_NumMeshTypes;
}

Texture2D<uint> g_MaterialIDTexture : register(t0);
Buffer<uint> g_MeshTypePerMaterialIDBuffer : register(t1);

float Main(PSInput input) : SV_Depth
{
	uint2 texturePos = uint2(input.screenSpacePos.xy);

	uint materialID = g_MaterialIDTexture[texturePos].r;
	uint meshType = g_MeshTypePerMaterialIDBuffer[materialID];

	return float(meshType) / float(g_NumMeshTypes);
}
