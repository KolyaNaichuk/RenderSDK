#ifndef __GAMMA_CORRECTION__
#define __GAMMA_CORRECTION__

static const float g_Gamma = 2.2f;

float3 GammaCorrection(float3 color)
{
	return pow(color, 1.0f / g_Gamma);
}

float3 GammaUncorrection(float3 color)
{
	return pow(color, g_Gamma);
}

#endif // __GAMMA_CORRECTION__
