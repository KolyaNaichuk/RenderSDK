struct VSInput
{
	uint vertexId		: SV_VertexID;
};

struct VSOutput
{
	float4 clipSpacePos : SV_Position;
	float2 texCoord		: TEXCOORD0;
};

cbuffer MeshTypeDataBuffer : register(b0)
{
	uint g_MeshType;
	uint g_NumMeshTypes;
};

StructuredBuffer<uint4> g_ShadingRectangleBuffer : register(t0);

VSOutput Main(VSInput input)
{
	float4 screenRect = g_ShadingRectangleBuffer[g_MeshType];
	
	float2 screenSpaceCorner = float2(0.0f, 0.0f);
	if (input.vertexId == 0)
		screenSpaceCorner = screenRect.xw;
	else if (input.vertexId == 1)
		screenSpaceCorner = screenRect.xy;
	else if (input.vertexId == 2)
		screenSpaceCorner = screenRect.zw;
	else if (input.vertexId == 3)
		screenSpaceCorner = screenRect.zy;

	float2 texSpaceCorner = (screenSpaceCorner.xy + 0.5f) * g_AppData.rcpScreenSize;
	float2 clipSpaceXY = float2(2.0f * texSpaceCorner.x - 1.0f, 1.0f - 2.0f * texSpaceCorner.y);
	float clipSpaceDepth = float(g_MeshType) / float(g_NumMeshTypes);

	VSOutput output;
	output.clipSpacePos = float4(clipSpaceXY, clipSpaceDepth, 1.0f);
	output.texCoord = texSpaceCorner;

	return output;
}