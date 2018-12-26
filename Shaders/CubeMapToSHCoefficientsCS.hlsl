#ifdef INTEGRATE

#include "Foundation.hlsl"
#include "SphericalHarmonics.hlsl"

Texture2DArray<float3> m_CubeMap : register(t0);
RWBuffer<float3> g_SumPerRowBuffer : register(u0);

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

groupshared float3 g_SharedMem[FACE_SIZE];

[numthreads(FACE_SIZE, 1, 1)]
void Main(uint3 localThreadId : SV_GroupThreadID, uint3 groupID : SV_GroupID)
{
	uint2 pixelPos = uint2(localThreadId.x, groupId.y);
	uint arraySlice = groupId.z;

	float3 pixelValue = m_CubeMap[uint3(pixelPos, arraySlice)];
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
	
	float3 worldSpaceDir = mul(g_RotationMatrices[arraySlice], localSpaceDir);
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

	g_SharedMem[localThreadId.x] = (SHValue * solidAngle) * pixelValue;
	GroupMemoryBarrierWithGroupSync();

	[unroll] for (uint offset = FACE_SIZE / 2; offset > 0; offset >>= 1)
	{
		if (localThreadId.x < offset)
			g_SharedMem[localThreadId.x] += g_SharedMem[localThreadId.x + offset];
		
		GroupMemoryBarrierWithGroupSync();
	}
	
	if (localThreadId.x == 0)
	{
		uint writeIndex = pixelPos.y * g_NumCubeMapFaces + arraySlice;
		g_SumPerRowBuffer[writeIndex] = g_SharedMem[0];
	}
}

#endif // INTEGRATE

#ifdef MERGE

#include "Foundation.hlsl"

cbuffer Constants32BitBuffer : register(b0)
{
	uint g_SHIndex;
}

Buffer<float3> g_SumPerRowBuffer : register(t0);
RWBuffer<float3> g_SHCoefficientBuffer : register(u0);

groupshared float3 g_SharedMem[FACE_SIZE];

[numthreads(1, FACE_SIZE, 1)]
void Main(uint3 localThreadId : SV_GroupThreadID)
{
	uint readOffset = localThreadId.y * g_NumCubeMapFaces;

	float3 sum = 0.0f;
	[unroll] for (uint faceIndex = 0; faceIndex < g_NumCubeMapFaces; ++faceIndex)
		sum += g_SumPerRowBuffer[readOffset + faceIndex];
	
	g_SharedMem[localThreadId.y] = sum;
	GroupMemoryBarrierWithGroupSync();

	[unroll] for (uint offset = FACE_SIZE / 2; offset > 0; offset >>= 1)
	{
		if (localThreadId.y < offset)
			g_SharedMem[localThreadId.y] += g_SharedMem[localThreadId.y + offset];

		GroupMemoryBarrierWithGroupSync();
	}

	if (localThreadId.y == 0)
		g_SHCoefficientBuffer[g_SHIndex] = g_SharedMem[0];
}

#endif // MERGE