#include "VoxelGrid.hlsl"

struct GSInput
{
	float4 worldSpacePos		: SV_Position;
	float3 worldSpaceNormal		: NORMAL;
	float2 texCoord				: TEXCOORD;
};

struct GSOutput
{
	float4 clipSpacePos			: SV_Position;
	float4 worldSpacePos		: POSITION;
	float3 worldSpaceNormal		: NORMAL;
	float2 texCoord				: TEXCOORD;
};

cbuffer CameraTransformBuffer : register(b0)
{
	CameraTransform g_CameraTransform;
}

static const float3 ViewDirections[3] = {
	float3(0.0f, 0.0f, 1.0f),
	float3(1.0f, 0.0f, 0.0f),
	float3(0.0f, 1.0f, 0.0f)
};

int FindViewDirectionWithLargestProjectedArea(float3 worldSpaceFaceNormal)
{
	float3x3 viewDirectionMatrix;
	viewDirectionMatrix[0] = ViewDirections[0];
	viewDirectionMatrix[1] = ViewDirections[1];
	viewDirectionMatrix[2] = ViewDirections[2];

	float3 dotProducts = abs(mul(worldSpaceFaceNormal, viewDirectionMatrix));
	float maxDotProduct = max(max(dotProducts.x, dotProducts.y), dotProducts.z);

	if (maxDotProduct == maxDotProduct.x)
		return 0;
	if (maxDotProduct == maxDotProduct.y)
		return 1;
	return 2;
}

[maxvertexcount(3)]
void Main(triangle GSInput input[3], inout TriangleStream<GSOutput> outputStream)
{
	float3 worldSpaceFaceNormal = normalize(input[0].worldSpaceNormal + input[1].worldSpaceNormal + input[2].worldSpaceNormal);
	int viewDirectionIndex = FindViewDirectionWithLargestProjectedArea(worldSpaceFaceNormal);
	
	for (int index = 0; index < 3; ++index)
	{
		GSOutput output;
		output.clipSpacePos = mul(input[index].worldSpacePos, g_CameraTransform.viewProjMatrices[viewDirectionIndex]);
		output.worldSpacePos = input[index].worldSpacePos;
		output.worldSpaceNormal = input[index].worldSpaceNormal;
		output.texCoord = input[index].texCoord;

		outputStream.Append(output);
	}
	outputStream.RestartStrip();
}