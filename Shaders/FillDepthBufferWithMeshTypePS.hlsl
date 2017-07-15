struct PSInput
{
	float4 screenSpacePos	: SV_Position;
	float2 texCoord			: TEXCOORD0;
};

cbuffer NumMeshTypesBuffer : register(b0)
{
	uint g_NumMeshTypes;
}

Texture2D<uint> g_GBuffer : register(t0); // Kolya. Not sure about GBuffer format
StructuredBuffer<uint> g_MeshTypePerMaterialIdBuffer : register(t1);

float Main(PSInput input) : SV_Depth
{
	int3 texturePos = int3(input.screenSpacePos.xy, 0);

	uint materialId = g_GBuffer[texturePos].r;
	uint meshType = g_MeshTypePerMaterialIdBuffer[materialId];

	return float(meshType) / float(g_NumMeshTypes);
}
