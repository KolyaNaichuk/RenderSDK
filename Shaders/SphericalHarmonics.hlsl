#ifndef __SPHERICAL_HARMONICS__
#define __SPHERICAL_HARMONICS__

struct SHSpectralCoeffs
{
	float4 r;
	float4 g;
	float4 b;
};

float4 SH(float3 dir)
{
	float4 coeffs;
	coeffs.x =  0.282095f;
	coeffs.y = -0.488603f * dir.y;
	coeffs.z =  0.488603f * dir.z;
	coeffs.w = -0.488603f * dir.x;

	return coeffs;
}

float4 SHProjectClampedCosine(float3 dir)
{
	float4 coeffs;
	coeffs.x =  0.88622695f;
	coeffs.y = -1.02332675f * dir.y;
	coeffs.z =  1.02332675f * dir.z;
	coeffs.w = -1.02332675f * dir.x;

	return coeffs;
}

#endif // __SPHERICAL_HARMONICS__