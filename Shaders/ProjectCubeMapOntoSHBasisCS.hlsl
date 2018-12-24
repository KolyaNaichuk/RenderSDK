#ifdef INTEGRATE_CUBEMAP_FACE

#include "LightUtils.hlsl"

Texture2DArray<float3> m_CubeMap : register(t0);
RWBuffer<float3> g_SumPerRowBuffer : register(u0);

static const float3x3 g_RotationMatrices[NUM_CUBEMAP_FACES] =
{
	// CUBEMAP_FACE_POSITIVE_X
	{
		0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f,
		-1.0f, 0.0f, 0.0f
	},
	// CUBEMAP_FACE_NEGATIVE_X
	{
		0.0f, 0.0f, -1.0f,
		0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
	}, 
	// CUBEMAP_FACE_POSITIVE_Y
	{
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, -1.0f, 0.0f
	},
	// CUBEMAP_FACE_NEGATIVE_Y
	{
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 1.0f, 0.0f
	},
	// CUBEMAP_FACE_POSITIVE_Z
	{
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f
	},
	// CUBEMAP_FACE_NEGATIVE_Z
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
	
#if COEFFICIENT_INDEX == 0
	float SHValue = 0.282095f;
#elif COEFFICIENT_INDEX == 1
#elif COEFFICIENT_INDEX == 2
#elif COEFFICIENT_INDEX == 3
#elif COEFFICIENT_INDEX == 4
#elif COEFFICIENT_INDEX == 5
#elif COEFFICIENT_INDEX == 6
#elif COEFFICIENT_INDEX == 7
#elif COEFFICIENT_INDEX == 8
#endif

	g_SharedMem[localThreadId.x] = (SHValue * solidAngle) * pixelValue;
	GroupMemoryBarrierWithGroupSync();

	for (uint offset = FACE_SIZE / 2; offset > 0; offset >>= 1)
	{
		if (localThreadId.x < offset)
			g_SharedMem[localThreadId.x] += g_SharedMem[localThreadId.x + offset];
		
		GroupMemoryBarrierWithGroupSync();
	}
	
	if (localThreadId.x == 0)
	{
		uint writeIndex = pixelPos.y * NUM_CUBEMAP_FACES + arraySlice;
		g_SumPerRowBuffer[writeIndex] = g_SharedMem[0];
	}
}

#endif // INTEGRATE_CUBEMAP_FACE

#ifdef REDUCE

#include "LightUtils.hlsl"

cbuffer Constants32BitBuffer : register(b0)
{
	uint g_CoefficientIndex;
}

Buffer<float3> g_SumPerRowBuffer : register(t0);
RWBuffer<float3> g_SHCoefficientBuffer : register(u0);

groupshared float3 g_SharedMem[FACE_SIZE];

[numthreads(1, FACE_SIZE, 1)]
void Main(uint3 localThreadId : SV_GroupThreadID)
{
	uint readOffset = localThreadId.y * NUM_CUBEMAP_FACES;

	float3 sum0 = g_SumPerRowBuffer[readOffset + 0];
	float3 sum1 = g_SumPerRowBuffer[readOffset + 1];
	float3 sum2 = g_SumPerRowBuffer[readOffset + 2];
	float3 sum3 = g_SumPerRowBuffer[readOffset + 3];
	float3 sum4 = g_SumPerRowBuffer[readOffset + 4];
	float3 sum5 = g_SumPerRowBuffer[readOffset + 5];
	
	g_SharedMem[localThreadId.y] = sum0 + sum1 + sum2 + sum3 + sum4 + sum5;
	GroupMemoryBarrierWithGroupSync();

	for (uint offset = FACE_SIZE / 2; offset > 0; offset >>= 1)
	{
		if (localThreadId.y < offset)
			g_SharedMem[localThreadId.y] += g_SharedMem[localThreadId.y + offset];

		GroupMemoryBarrierWithGroupSync();
	}

	if (localThreadId.y == 0)
		g_SHCoefficientBuffer[g_CoefficientIndex] = g_SharedMem[0];
}

#endif // REDUCE