struct GSInput
{
	float4 worldSpacePos		: SV_Position;
	float3 worldSpaceNormal		: NORMAL;
};

struct GSOutput
{
	float4 clipSpacePos			: SV_Position;
	float3 worldSpacePos		: POSITION;
	float3 worldSpaceNormal		: NORMAL;
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
	
	int viewIndex = FindViewDirectionWithLargestProjectedArea(worldSpaceFaceNormal);
	float4x4 viewProjMatrix = g_Transform.viewProjMatrices[viewIndex];

	for (int index = 0; index < 3; ++index)
	{
		GSOutput output;
		output.clipSpacePos = mul(viewProjMatrix, input[index].worldSpacePos);
		output.worldSpacePos = input[index].worldSpacePos.xyz;
		output.worldSpaceNormal = input[index].worldSpaceNormal;

		outputStream.Append(output);
	}
	outputStream.RestartStrip();
}
