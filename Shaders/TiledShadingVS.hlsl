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

StructuredBuffer<uint2> g_ShadingRectangleMinPointBuffer : register(t0);
StructuredBuffer<uint2> g_ShadingRectangleMaxPointBuffer : register(t1);

VSOutput Main(VSInput input)
{
	float2 minPoint = g_ShadingRectangleMinPointBuffer[g_MeshType];
	float2 maxPoint = g_ShadingRectangleMaxPointBuffer[g_MeshType];
	
	float2 screenSpaceCorner = float2(0.0f, 0.0f);
	if (input.vertexId == 0)
		screenSpaceCorner = float2(minPoint.x, maxPoint.y);
	else if (input.vertexId == 1)
		screenSpaceCorner = minPoint;
	else if (input.vertexId == 2)
		screenSpaceCorner = maxPoint;
	else if (input.vertexId == 3)
		screenSpaceCorner = float2(maxPoint.x, minPoint.y);

	float2 texSpaceCorner = (screenSpaceCorner.xy + 0.5f) * g_AppData.rcpScreenSize;
	float2 clipSpaceXY = float2(2.0f * texSpaceCorner.x - 1.0f, 1.0f - 2.0f * texSpaceCorner.y);
	float clipSpaceDepth = float(g_MeshType) / float(g_NumMeshTypes);

	VSOutput output;
	output.clipSpacePos = float4(clipSpaceXY, clipSpaceDepth, 1.0f);
	output.texCoord = texSpaceCorner;

	return output;
}