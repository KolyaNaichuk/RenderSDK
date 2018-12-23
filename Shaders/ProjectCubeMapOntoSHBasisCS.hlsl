#ifdef INTEGRATE_CUBE_MAP_FACE

#include "LightUtils.hlsl"

Texture2DArray<float3> m_CubeMap : register(t0);

static const float3x3 g_RotationMatrices[NUM_CUBE_MAP_FACES] =
{
	matrix(), // CUBE_MAP_FACE_POS_X
	matrix(), // CUBE_MAP_FACE_NEG_X
	matrix(), // CUBE_MAP_FACE_POS_Y
	matrix(), // CUBE_MAP_FACE_NEG_Y
	
	{1.0f, 0.0f, 0.0f,		// CUBE_MAP_FACE_POS_Z
	 0.0f, 1.0f, 0.0f,
	 0.0f, 0.0f, 1.0f},

	matrix(), // CUBE_MAP_FACE_NEG_Z
};

groupshared float3 g_CachedData[FACE_SIZE];

[numthreads(FACE_SIZE, 1, 1)]
void Main(uint3 localThreadId : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
	uint2 pixelPos = uint2(localThreadId.x, groupId.y);
	uint arraySlice = groupId.z;

	float3 pixelValue = m_CubeMap[uint3(pixelPos, arraySlice)];
	
	float rcpHalfFaceSize = 2.0f / float(FACE_SIZE);
	float3 localSpaceDir = float3(rcpHalfFaceSize * (float(pixelPos.x) + 0.5f) - 1.0f,
		-rcpHalfFaceSize * (float(pixelPos.y) + 0.5f) + 1.0f,
		1.0f
	);

	float factor = dot(localSpaceDir, localSpaceDir);
	float solidAngle = 4.0f / (factor * sqrt(factor));
	float3 weightedPixelValue = solidAngle * pixelValue;

	float3 worldSpaceDir = mul(g_RotationMatrices[arraySlice], localSpaceDir);
	worldSpaceDir = normalize(worldSpaceDir);

	float SHBasisFuncValues[9];
	SH9EvaluateBasisFunctions(SHBasisFuncValues, worldSpaceDir);

	g_CachedData[pixelPos.x] = SHBasisFuncValues[0] * weightedPixelValue;
}

#endif // INTEGRATE_CUBE_MAP_FACE