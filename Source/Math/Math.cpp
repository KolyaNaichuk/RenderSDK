#include "Math/Math.h"

f32 ToDegrees(f32 radians)
{
	static const f32 radiansToDegrees = 180.0f / PI;
	return radiansToDegrees * radians;
}

f32 ToRadians(f32 degrees)
{
	static const f32 degreesToRadians = PI / 180.0f;
	return degreesToRadians * degrees;
}

f32 Sin(f32 angleInRadians)
{
	return std::sinf(angleInRadians);
}

f32 ArcSin(f32 sinAngle)
{
	f32 angleInRadians = std::asinf(sinAngle);
	return angleInRadians;
}

f32 Cos(f32 angleInRadians)
{
	return std::cosf(angleInRadians);
}

f32 ArcCos(f32 cosAngle)
{
	f32 angleInRadians = std::acosf(cosAngle);
	return angleInRadians;
}

f32 Tan(f32 angleInRadians)
{
	return std::tanf(angleInRadians);
}

f32 ArcTan(f32 tanAngle)
{
	f32 angleInRadians = std::atanf(tanAngle);
	return angleInRadians;
}

void SinCos(f32& sinAngle, f32& cosAngle, f32 angleInRadians)
{
	sinAngle = Sin(angleInRadians);
	cosAngle = Cos(angleInRadians);
}

f32 Ceil(f32 value)
{
	return std::ceilf(value);
}

f64 Ceil(f64 value)
{
	return std::ceil(value);
}

f32 Floor(f32 value)
{
	return std::floorf(value);
}

f64 Floor(f64 value)
{
	return std::floor(value);
}

f32 Pow(f32 base, f32 exponent)
{
	return std::powf(base, exponent);
}

f64 Pow(f64 base, f64 exponent)
{
	return std::pow(base, exponent);
}

f32 Sqrt(f32 value)
{
	return std::sqrtf(value);
}

f64 Sqrt(f64 value)
{
	return std::sqrt(value);
}

f32 Lerp(f32 minValue, f32 maxValue, f32 weight)
{
	return minValue + weight * (maxValue - minValue);
}

f64 Lerp(f64 minValue, f64 maxValue, f64 weight)
{
	return minValue + weight * (maxValue - minValue);
}

f32 SmoothStep(f32 minValue, f32 maxValue, f32 value)
{
	f32 x = Saturate((value - minValue) / (maxValue - minValue));
	return (3.0f - 2.0f * x) * x * x;
}

f64 SmoothStep(f64 minValue, f64 maxValue, f64 value)
{
	f64 x = Saturate((value - minValue) / (maxValue - minValue));
	return (3.0 - 2.0 * x) * x * x;
}

f32 SmootherStep(f32 minValue, f32 maxValue, f32 value)
{
	f32 x = Saturate((value - minValue) / (maxValue - minValue));
	return (10.0f + x * (6.0f * x - 15.0f)) * x * x * x;
}

f64 SmootherStep(f64 minValue, f64 maxValue, f64 value)
{
	f64 x = Saturate((value - minValue) / (maxValue - minValue));
	return (10.0 + x * (6.0 * x - 15.0)) * x * x * x;
}

f32 SmoothestStep(f32 minValue, f32 maxValue, f32 value)
{
	f32 x = Saturate((value - minValue) / (maxValue - minValue));
	return ((35.0f - 84.0f * x + 70.0f * x * x - 20.0f * x * x * x) * x * x * x * x);
}

f64 SmoothestStep(f64 minValue, f64 maxValue, f64 value)
{
	f64 x = Saturate((value - minValue) / (maxValue - minValue));
	return ((35.0 - 84.0 * x + 70.0 * x * x - 20.0 * x * x * x) * x * x * x * x);
}
