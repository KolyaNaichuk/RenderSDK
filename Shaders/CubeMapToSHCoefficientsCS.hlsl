#include "SphericalHarmonics.hlsl"

uint ComputeSHMapIndex(uint SHIndex, uint faceIndex)
{
	return (SHIndex * g_NumCubeMapFaces + faceIndex);
}

uint ComputeIntegratedDataOffset(uint SHIndex, uint rowIndex)
{
	return (SHIndex * FACE_SIZE + rowIndex) * g_NumCubeMapFaces;
}

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

RWStructuredBuffer<float3> g_SumPerRowBuffer : register(u0);
Texture2DArray<float> g_WeightedSHMap : register(t0);
Texture2DArray<float4> g_CubeMap : register(t1);

groupshared float3 g_SharedMem[FACE_SIZE];

[numthreads(FACE_SIZE, 1, 1)]
void Main(uint3 localThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID)
{
	uint faceIndex = groupId.z;
	uint2 pixelPos = uint2(localThreadId.x, groupId.y);
	float3 pixelValue = g_CubeMap[uint3(pixelPos, faceIndex)].rgb;

	uint SHIndex = groupId.x;
	uint SHMapIndex = ComputeSHMapIndex(SHIndex, faceIndex);
	float weightedSHValue = g_WeightedSHMap[uint3(pixelPos, SHMapIndex)];

	g_SharedMem[localThreadId.x] = weightedSHValue * pixelValue;
	[unroll] for (uint offset = FACE_SIZE / 2; offset > 0; offset >>= 1)
	{
		GroupMemoryBarrierWithGroupSync();

		if (localThreadId.x < offset)
			g_SharedMem[localThreadId.x] += g_SharedMem[localThreadId.x + offset];
	}
	
	if (localThreadId.x == 0)
	{
		uint dataOffset = ComputeIntegratedDataOffset(SHIndex, pixelPos.y);
		g_SumPerRowBuffer[dataOffset + faceIndex] = g_SharedMem[0];
	}
}

#endif // INTEGRATE

#ifdef MERGE

StructuredBuffer<float3> g_SumPerRowBuffer : register(t0);
RWStructuredBuffer<float3> g_SHCoefficientBuffer : register(u0);

groupshared float3 g_SharedMem[FACE_SIZE];

[numthreads(1, FACE_SIZE, 1)]
void Main(uint3 localThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID)
{
	uint SHIndex = groupId.x;
	uint dataOffset = ComputeIntegratedDataOffset(SHIndex, localThreadId.y);

	float3 sum = 0.0f;
	[unroll] for (uint faceIndex = 0; faceIndex < g_NumCubeMapFaces; ++faceIndex)
		sum += g_SumPerRowBuffer[dataOffset + faceIndex];
	
	g_SharedMem[localThreadId.y] = sum;
	[unroll] for (uint offset = FACE_SIZE / 2; offset > 0; offset >>= 1)
	{
		GroupMemoryBarrierWithGroupSync();

		if (localThreadId.y < offset)
			g_SharedMem[localThreadId.y] += g_SharedMem[localThreadId.y + offset];
	}

	if (localThreadId.y == 0)
		g_SHCoefficientBuffer[SHIndex] = g_SharedMem[0];
}

#endif // MERGE