#include "SphericalHarmonics.hlsl"

uint ComputeSHMapIndex(uint SHIndex, uint faceIndex)
{
	return (SHIndex * g_NumCubeMapFaces + faceIndex);
}

#define FACE_SIZE_DIV_2		(FACE_SIZE / 2)
#define FACE_SIZE_DIV_4		(FACE_SIZE / 4)

#ifdef PRECOMPUTE

static const float3x3 g_RotationMatrices[g_NumCubeMapFaces] =
{
	// Positive X
	{
		0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f,
		-1.0f, 0.0f, 0.0f
	},
	// Negative X
	{
		0.0f, 0.0f, -1.0f,
		0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
	},
	// Positive Y
	{
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, -1.0f, 0.0f
	},
	// Negative Y
	{
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 1.0f, 0.0f
	},
	// Positive Z
	{
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f
	},
	// Negative Z
	{
		-1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, -1.0f
	}
};

RWTexture2DArray<float> g_WeightedSHMap : register(u0);

[numthreads(FACE_SIZE, 1, 1)]
void Main(uint3 localThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID)
{
	uint faceIndex = groupId.z;
	uint2 pixelPos = uint2(localThreadId.x, groupId.y);
	
	float rcpHalfFaceSize = 2.0f / float(FACE_SIZE);

	// The following calculations are based on assumption
	// the cube map is pre-transformed to span from -1 to 1.

	float3 localSpaceDir = float3(rcpHalfFaceSize * (float(pixelPos.x) + 0.5f) - 1.0f,
		-rcpHalfFaceSize * (float(pixelPos.y) + 0.5f) + 1.0f,
		1.0f
	);
	float pixelArea = rcpHalfFaceSize * rcpHalfFaceSize;
	float sphereMappingJacobian = 1.0f / pow(dot(localSpaceDir, localSpaceDir), 1.5f);
	float solidAngle = pixelArea * sphereMappingJacobian;

	float3 worldSpaceDir = mul(g_RotationMatrices[faceIndex], localSpaceDir);
	worldSpaceDir = normalize(worldSpaceDir);

#if SH_INDEX == 0
	float SHValue = SH0();
#elif SH_INDEX == 1
	float SHValue = SH1(worldSpaceDir);
#elif SH_INDEX == 2
	float SHValue = SH2(worldSpaceDir);
#elif SH_INDEX == 3
	float SHValue = SH3(worldSpaceDir);
#elif SH_INDEX == 4
	float SHValue = SH4(worldSpaceDir);
#elif SH_INDEX == 5
	float SHValue = SH5(worldSpaceDir);
#elif SH_INDEX == 6
	float SHValue = SH6(worldSpaceDir);
#elif SH_INDEX == 7
	float SHValue = SH7(worldSpaceDir);
#elif SH_INDEX == 8
	float SHValue = SH8(worldSpaceDir);
#endif

	uint SHMapIndex = ComputeSHMapIndex(SH_INDEX, faceIndex);
	g_WeightedSHMap[uint3(pixelPos, SHMapIndex)] = SHValue * solidAngle;
}

#endif // PRECOMPUTE

#ifdef INTEGRATE

RWStructuredBuffer<float3> g_SumPerColumnBuffer : register(u0);
Texture2DArray<float> g_WeightedSHMap : register(t0);
Texture2DArray<float4> g_CubeMap : register(t1);
SamplerState g_Sampler : register(s0);

groupshared float3 g_SharedMem[FACE_SIZE];

[numthreads(1, FACE_SIZE, 1)]
void Main(uint3 localThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID)
{
	uint faceIndex = groupId.z;
	float2 texCoords = (float2(groupId.x << 2, localThreadId.y) + 0.5f) / float(FACE_SIZE);

	int2 offset0 = int2(0, 0);
	int2 offset1 = int2(1, 0);
	int2 offset2 = int2(2, 0);
	int2 offset3 = int2(3, 0);

	float4 redValues = g_CubeMap.GatherRed(g_Sampler,
		float3(texCoords, faceIndex), offset0, offset1, offset2, offset3);

	float4 greenValues = g_CubeMap.GatherGreen(g_Sampler,
		float3(texCoords, faceIndex), offset0, offset1, offset2, offset3);

	float4 blueValues = g_CubeMap.GatherBlue(g_Sampler,
		float3(texCoords, faceIndex), offset0, offset1, offset2, offset3);

	uint SHIndex = groupId.y;
	uint SHMapIndex = ComputeSHMapIndex(SHIndex, faceIndex);

	float4 weightedSHValues = g_WeightedSHMap.GatherRed(g_Sampler,
		float3(texCoords, SHMapIndex), offset0, offset1, offset2, offset3);

	float3 sum;
	sum.r = dot(weightedSHValues, redValues);
	sum.g = dot(weightedSHValues, greenValues);
	sum.b = dot(weightedSHValues, blueValues);

	g_SharedMem[localThreadId.y] = sum;
	[unroll] for (uint offset = FACE_SIZE_DIV_2; offset > 0; offset >>= 1)
	{
		GroupMemoryBarrierWithGroupSync();

		if (localThreadId.y < offset)
			g_SharedMem[localThreadId.y] += g_SharedMem[localThreadId.y + offset];
	}
	
	if (localThreadId.y == 0)
	{
		uint writeOffset = FACE_SIZE_DIV_4 * (SHIndex * g_NumCubeMapFaces + faceIndex) + groupId.x;
		g_SumPerColumnBuffer[writeOffset] = g_SharedMem[0];
	}
}

#endif // INTEGRATE

#ifdef MERGE

StructuredBuffer<float3> g_SumPerColumnBuffer : register(t0);
RWStructuredBuffer<float3> g_SHCoefficientBuffer : register(u0);

groupshared float3 g_SharedMem[FACE_SIZE];

[numthreads(FACE_SIZE, 1, 1)]
void Main(uint3 localThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID)
{
	uint SHIndex = groupId.y;

	uint readOffset = SHIndex * g_NumCubeMapFaces * FACE_SIZE_DIV_4 + localThreadId.x;
	float3 sum = g_SumPerColumnBuffer[readOffset];
	if (localThreadId.x < FACE_SIZE_DIV_2)
		sum += g_SumPerColumnBuffer[readOffset + FACE_SIZE_DIV_2];
		
	g_SharedMem[localThreadId.x] = sum;
	[unroll] for (uint offset = FACE_SIZE_DIV_2; offset > 0; offset >>= 1)
	{
		GroupMemoryBarrierWithGroupSync();

		if (localThreadId.x < offset)
			g_SharedMem[localThreadId.x] += g_SharedMem[localThreadId.x + offset];
	}

	if (localThreadId.x == 0)
		g_SHCoefficientBuffer[SHIndex] = g_SharedMem[0];
}

#endif // MERGE