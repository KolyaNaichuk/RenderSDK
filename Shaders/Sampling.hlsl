#ifndef __SAMPLING__
#define __SAMPLING__

#include "Foundation.hlsl"

float RadicalInverse(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

	return float(bits) * 2.3283064365386963e-10;
}

// Generates uniformly distributed 2D sample in range [0, 1) x [0, 1)
float2 Hammersley(uint sampleIndex, uint numSamples)
{
	float E1 = float(sampleIndex) / float(numSamples);
	float E2 = RadicalInverse(sampleIndex);

	return float2(E1, E2);
}

// Generates light sample direction in local coordinate space.
// By assumption, N is (0, 0, 1) in this space.
float3 CosineSampleHemisphere(float2 E)
{
	float cosTheta = sqrt(1.0f - E.x);
	float sinTheta = sqrt(E.x);
	float phi = g_2PI * E.y;

	float3 L;
	L.x = sinTheta * cos(phi);
	L.y = sinTheta * sin(phi);
	L.z = cosTheta;

	return L;
}

// Generates microfacet orientation sample (H) from GGX distribution in local coordinate space.
// By assumption, N is (0, 0, 1) in this space.
float3 SampleGGX(float2 E, float squaredRoughness)
{
	float cosTheta = sqrt((1.0f - E.x) / ((squaredRoughness - 1.0f) * E.x + 1.0f));
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
	float phi = g_2PI * E.y;
	
	float3 H;
	H.x = sinTheta * cos(phi);
	H.y = sinTheta * sin(phi);
	H.z = cosTheta;
	
	return H;
}

#endif // __SAMPLING__