#ifndef __RAY_TRACING_UTILS__
#define __RAY_TRACING_UTILS__

float InterpolateTriangleAttributes(float attributes[3], float2 barycentrics)
{
	return (attributes[0] + barycentrics.x * (attributes[1] - attributes[0]) + barycentrics.y * (attributes[2] - attributes[0]));
}

float2 InterpolateTriangleAttributes(float2 attributes[3], float2 barycentrics)
{
	return (attributes[0] + barycentrics.x * (attributes[1] - attributes[0]) + barycentrics.y * (attributes[2] - attributes[0]));
}

float3 InterpolateTriangleAttributes(float3 attributes[3], float2 barycentrics)
{
	return (attributes[0] + barycentrics.x * (attributes[1] - attributes[0]) + barycentrics.y * (attributes[2] - attributes[0]));
}

float4 InterpolateTriangleAttributes(float4 attributes[3], float2 barycentrics)
{
	return (attributes[0] + barycentrics.x * (attributes[1] - attributes[0]) + barycentrics.y * (attributes[2] - attributes[0]));
}

#endif // __RAY_TRACING_UTILS__