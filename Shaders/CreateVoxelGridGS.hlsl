#include "VoxelGrid.hlsl"

struct GSInput
{
	float4 worldSpacePos		: SV_Position;
	float3 worldSpaceNormal		: NORMAL;

#ifdef HAS_TEXCOORD
	float2 texCoord				: TEXCOORD;
#else // HAS_COLOR
	float4 color				: COLOR;
#endif // HAS_TEXCOORD
};

struct GSOutput
{
	float4 clipSpacePos			: SV_Position;
	float4 worldSpacePos		: POSITION;
	float3 worldSpaceNormal		: NORMAL;

#ifdef HAS_TEXCOORD
	float2 texCoord				: TEXCOORD;
#else // HAS_COLOR
	float4 color				: COLOR;
#endif // HAS_TEXCOORD
};

cbuffer TransformBuffer : register(b0)
{
	CameraTransform g_Transform;
}

int FindViewDirectionWithLargestProjectedArea(float3 worldSpaceFaceNormal)
{
	float3 dotProducts;
	dotProducts.x = abs(dot(worldSpaceFaceNormal, float3(1.0f, 0.0f, 0.0f)));
	dotProducts.y = abs(dot(worldSpaceFaceNormal, float3(0.0f, 1.0f, 0.0f)));
	dotProducts.z = abs(dot(worldSpaceFaceNormal, float3(0.0f, 0.0f, 1.0f)));
	
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
	int viewDirectionIndex = FindViewDirectionWithLargestProjectedArea(worldSpaceFaceNormal);
	
	for (int index = 0; index < 3; ++index)
	{
		GSOutput output;
		output.clipSpacePos = mul(input[index].worldSpacePos, g_Transform.viewProjMatrices[viewDirectionIndex]);
		output.worldSpacePos = input[index].worldSpacePos;
		output.worldSpaceNormal = input[index].worldSpaceNormal;

#ifdef HAS_TEXCOORD
		output.texCoord = input[index].texCoord;
#else // HAS_COLOR
		output.color = input[index].color;
#endif // HAS_TEXCOORD

		outputStream.Append(output);
	}
	outputStream.RestartStrip();
}
