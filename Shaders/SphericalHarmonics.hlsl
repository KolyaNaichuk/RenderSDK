#ifndef __SPHERICAL_HARMONICS__
#define __SPHERICAL_HARMONICS__

#include "Foundation.hlsl"

// Band 0
float SH0() { return 0.282095f; }

// Band 1
float SH1(float3 normDir) { return -0.488603f * normDir.y; }
float SH2(float3 normDir) { return 0.488603f * normDir.z; }
float SH3(float3 normDir) { return -0.488603f * normDir.x; }

// Band 2
float SH4(float3 normDir) { return 1.092548f * normDir.x * normDir.y; }
float SH5(float3 normDir) { return -1.092548f * normDir.y * normDir.z; }
float SH6(float3 normDir) { return 0.315392f * (3.0f * normDir.z * normDir.z - 1.0f); }
float SH7(float3 normDir) { return -1.092548f * normDir.x * normDir.z; }
float SH8(float3 normDir) { return 0.546274f * (normDir.x * normDir.x - normDir.y * normDir.y); }

float3 SH9Reconstruct(in float3 SHCoeffs[9], in float3 normDir)
{
	// Band 0
	float3 result = SHCoeffs[0] * SH0();
	
	// Band 1
	result += SHCoeffs[1] * SH1(normDir);
	result += SHCoeffs[2] * SH2(normDir);
	result += SHCoeffs[3] * SH3(normDir);
	
	// Band 2
	result += SHCoeffs[4] * SH4(normDir);
	result += SHCoeffs[5] * SH5(normDir);
	result += SHCoeffs[6] * SH6(normDir);
	result += SHCoeffs[7] * SH7(normDir);
	result += SHCoeffs[8] * SH8(normDir);

	return result;
}

void SH9RadianceToIrradianceCoefficients(out float3 SHIrradianceCoeffs[9], in float3 SHRadianceCoeffs[9])
{
	// Based on article An Efficient Representation for Irradiance Environment Maps
	// by Ravi Ramamoorthi and Pat Hanrahan. Formulae 5 and 9.

	const float A0 = 3.141593f;
	const float A1 = 2.094395f;
	const float A2 = 0.785398f;

	// Band 0
	SHIrradianceCoeffs[0] = A0 * SHRadianceCoeffs[0];

	// Band 1
	SHIrradianceCoeffs[1] = A1 * SHRadianceCoeffs[1];
	SHIrradianceCoeffs[2] = A1 * SHRadianceCoeffs[2];
	SHIrradianceCoeffs[3] = A1 * SHRadianceCoeffs[3];

	// Band 2
	SHIrradianceCoeffs[4] = A2 * SHRadianceCoeffs[4];
	SHIrradianceCoeffs[5] = A2 * SHRadianceCoeffs[5];
	SHIrradianceCoeffs[6] = A2 * SHRadianceCoeffs[6];
	SHIrradianceCoeffs[7] = A2 * SHRadianceCoeffs[7];
	SHIrradianceCoeffs[8] = A2 * SHRadianceCoeffs[8];
}

#endif // __SPHERICAL_HARMONICS__