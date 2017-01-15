#ifndef __SPHERICAL_HARMONICS__
#define __SPHERICAL_HARMONICS__

typedef float4 SHCoeffs;

struct SHSpectralCoeffs
{
	SHCoeffs r;
	SHCoeffs g;
	SHCoeffs b;
};

SHCoeffs SHProjectClampedCosine(float3 dir)
{
	SHCoeffs coeffs;
	coeffs.x =  0.88622695f;
	coeffs.y = -1.02332675f * dir.y;
	coeffs.z =  1.02332675f * dir.z;
	coeffs.w = -1.02332675f * dir.x;

	return coeffs;
}

#endif // __SPHERICAL_HARMONICS__