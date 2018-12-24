#ifndef __SPHERICAL_HARMONICS__
#define __SPHERICAL_HARMONICS__

#include "Foundation.hlsl"

void SH9EvaluateBasisFunctions(out float SHBasisFuncValues[9], in float3 normDir)
{
	// Band 1
	SHBasisFuncValues[0] =  0.282095f;

	// Band 2
	SHBasisFuncValues[1] = -0.488603f * normDir.y;
	SHBasisFuncValues[2] =  0.488603f * normDir.z;
	SHBasisFuncValues[3] = -0.488603f * normDir.x;

	// Band 3
	float3 squaredNormDir = normDir * normDir;
	SHBasisFuncValues[4] =  1.092548f * normDir.x * normDir.y;
	SHBasisFuncValues[5] = -1.092548f * normDir.y * normDir.z;
	SHBasisFuncValues[6] =  0.315392f * (3.0f * squaredNormDir.z - 1.0f);
	SHBasisFuncValues[7] = -1.092548f * normDir.x * normDir.z;
	SHBasisFuncValues[8] =  0.546274f * (squaredNormDir.x - squaredNormDir.y);
}

float3 SH9Reconstruct(in float3 SHProjCoeffs[9], in float3 normDir)
{
	float SHBasisFuncValues[9];
	SH9EvaluateBasisFunctions(SHBasisFuncValues, normDir);

	float3 result = 0.0f;

	[unroll]
	for (uint i = 0; i < 9; ++i)
		result += SHProjCoeffs[i] * SHBasisFuncValues[i];

	return result;
}

#endif // __SPHERICAL_HARMONICS__