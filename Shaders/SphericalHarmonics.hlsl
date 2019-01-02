#ifndef __SPHERICAL_HARMONICS__
#define __SPHERICAL_HARMONICS__

#include "Foundation.hlsl"

// Band 1
float SH0() { return 0.282095f; }

// Band 2
float SH1(float3 normDir) { return -0.488603f * normDir.y; }
float SH2(float3 normDir) { return 0.488603f * normDir.z; }
float SH3(float3 normDir) { return -0.488603f * normDir.x; }

// Band 3
float SH4(float3 normDir) { return 1.092548f * normDir.x * normDir.y; }
float SH5(float3 normDir) { return -1.092548f * normDir.y * normDir.z; }
float SH6(float3 normDir) { return 0.315392f * (3.0f * normDir.z * normDir.z - 1.0f); }
float SH7(float3 normDir) { return -1.092548f * normDir.x * normDir.z; }
float SH8(float3 normDir) { return 0.546274f * (normDir.x * normDir.x - normDir.y * normDir.y); }

void SH9EvaluateBasisFunctions(out float SHBasisFuncValues[9], in float3 normDir)
{
	// Band 1
	SHBasisFuncValues[0] = SH0();

	// Band 2
	SHBasisFuncValues[1] = SH1(normDir);
	SHBasisFuncValues[2] = SH2(normDir);
	SHBasisFuncValues[3] = SH3(normDir);

	// Band 3
	SHBasisFuncValues[4] = SH4(normDir);
	SHBasisFuncValues[5] = SH5(normDir);
	SHBasisFuncValues[6] = SH6(normDir);
	SHBasisFuncValues[7] = SH7(normDir);
	SHBasisFuncValues[8] = SH8(normDir);
}

float3 SH9Reconstruct(in float3 SHCoeffs[9], in float3 normDir)
{
	float SHBasisFuncValues[9];
	SH9EvaluateBasisFunctions(SHBasisFuncValues, normDir);

	float3 result = 0.0f;

	[unroll]
	for (uint i = 0; i < 9; ++i)
		result += SHCoeffs[i] * SHBasisFuncValues[i];

	return result;
}

void SH9RadianceToIrradianceCoefficients(out float3 SHIrradianceCoeffs, in float3 SHRadianceCoeffs[9])
{
	Add implementation
}

#endif // __SPHERICAL_HARMONICS__