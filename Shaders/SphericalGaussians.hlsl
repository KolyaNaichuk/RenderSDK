#ifndef __SPHERICAL_GAUSSIANS__
#define __SPHERICAL_GAUSSIANS__

#include "Foundation.hlsl"

struct SG
{
	float3 amplitude;
	float3 axis;
	float sharpness;
};

float3 SGEvaluate(SG sg, float3 dir)
{
	float cosAngle = dot(sg.axis, dir);
	return sg.amplitude * exp(sg.sharpness * (cosAngle - 1.0f));
}

SG SGProduct(SG sg1, SG sg2)
{
	float unnormSharpness = sg1.sharpness + sg2.sharpness;
	
	float3 unnormAxis = (sg1.sharpness * sg1.axis + sg2.sharpness * sg2.axis) /
		(sg1.sharpness + sg2.sharpness);
	
	float unnormAxisLength = length(unnormAxis);
	
	SG sg;
	sg.amplitude = sg1.amplitude * sg2.amplitude * exp(unnormSharpness * (unnormAxisLength - 1.0f));
	sg.axis = unnormAxis / unnormAxisLength;
	sg.sharpness = unnormSharpness * unnormAxisLength;

	return sg;
}

float3 SGEvaluateIntegralOverEntireSphere(SG sg)
{
	float expTerm = 1.0f - exp(-2.0f * sg.sharpness);
	return (g_2PI * expTerm / sg.sharpness) * sg.amplitude;
}

float SGApproximateIntegralOverEntireSphere(SG sg)
{
	return (g_2PI / sg.sharpness) * sg.amplitude;
}

SG SGApproximateClampedCosineLobe(float3 direction)
{
	SG clampedCosineLobe;
	clampedCosineLobe.axis = direction;
	clampedCosineLobe.sharpness = 2.133f;
	clampedCosineLobe.amplitude = 1.17f;

	return clampedCosineLobe;
}

#endif // __SPHERICAL_GAUSSIANS__