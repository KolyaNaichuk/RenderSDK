#define LIGHT_TYPE_POINT	1
#define LIGHT_TYPE_SPOT		2

struct GSInput
{
	float4 worldSpacePos		: SV_Position;
	uint lightIndex				: LIGHT_INDEX;
};

struct GSOutput
{
	float4 clipSpacePos			: SV_Position;
};

#if LIGHT_TYPE == LIGHT_TYPE_POINT

[maxvertexcount(3)]
void Main(triangle GSInput input[3], inout TriangleStream<GSOutput> outputStream)
{
	const uint lightIndex = input[0].lightIndex;
}

#endif // #if LIGHT_TYPE == LIGHT_TYPE_POINT

#if LIGHT_TYPE == LIGHT_TYPE_SPOT

[maxvertexcount(3)]
void Main(triangle GSInput input[3], inout TriangleStream<GSOutput> outputStream)
{
	const uint lightIndex = input[0].lightIndex; 

	float3 worldSpaceFaceSide1 = input[1].worldSpacePos.xyz - input[0].worldSpacePos.xyz;
	float3 worldSpaceFaceSide2 = input[2].worldSpacePos.xyz - input[0].worldSpacePos.xyz;
	float3 worldSpaceFaceNormal = normalize(cross(worldSpaceFaceSide1, worldSpaceFaceSide2));
	
	bool isBackFace = (dot(worldSpaceFaceNormal, worldSpaceLightDir) > 0.0f);
	if (isBackFace)
		return;


}

#endif // #if LIGHT_TYPE == LIGHT_TYPE_SPOT