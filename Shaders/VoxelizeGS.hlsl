#include "Foundation.hlsl"

struct GSInput
{
	float4 worldSpacePos		: SV_Position;
	float3 worldSpaceNormal		: NORMAL;
	float2 texCoord				: TEXCOORD;
};

struct GSOutput
{
	float4 clipSpacePos			: SV_Position;
	float3 worldSpacePos		: POSITION;
	float3 worldSpaceNormal		: NORMAL;
	float2 texCoord				: TEXCOORD;
};

cbuffer AppDataBuffer : register(b0)
{
	AppData g_AppData;
}

int FindViewDirectionWithLargestProjectedArea(float3 worldSpaceFaceNormal)
{
	float3x3 viewDirectionMatrix =
	{
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f
	};
	float3 dotProducts = abs(mul(viewDirectionMatrix, worldSpaceFaceNormal));
	float maxDotProduct = max(max(dotProducts.x, dotProducts.y), dotProducts.z);
	if (maxDotProduct == dotProducts.x)
		return 0;
	if (maxDotProduct == dotProducts.y)
		return 1;
	return 2;
}

[maxvertexcount(3)]
void Main(triangle GSInput input[3], inout TriangleStream<GSOutput> outputStream)
{
	float3 worldSpaceFaceNormal = normalize(input[0].worldSpaceNormal + input[1].worldSpaceNormal + input[2].worldSpaceNormal);
	
	int viewIndex = FindViewDirectionWithLargestProjectedArea(worldSpaceFaceNormal);
	float4x4 viewProjMatrix = g_AppData.voxelGridViewProjMatrices[viewIndex];

	[unroll]
	for (int index = 0; index < 3; ++index)
	{
		GSOutput output;
		output.clipSpacePos = mul(viewProjMatrix, input[index].worldSpacePos);
		output.worldSpacePos = input[index].worldSpacePos.xyz;
		output.worldSpaceNormal = input[index].worldSpaceNormal;
		output.texCoord = input[index].texCoord;

		outputStream.Append(output);
	}
	outputStream.RestartStrip();
}
