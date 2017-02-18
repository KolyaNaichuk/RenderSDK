#pragma once

#include "Common/Common.h"
#include <cmath>

static const f32 PI = 3.141592654f;
static const f32 TWO_PI = 6.283185307f;
static const f32 RCP_PI = 0.318309886f;
static const f32 RCP_TWO_PI = 0.159154943f;
static const f32 PI_DIV_TWO = 1.570796327f;
static const f32 PI_DIV_FOUR = 0.785398163f;
static const f32 EPSILON = 1e-6f;

template <typename T>
bool IsInRange(T minValue, T maxValue, T value)
{
	return ((minValue <= value) && (value <= maxValue));
}

template <typename T>
T Max(T left, T right)
{
    return (left > right) ? left : right;
}

template <typename T>
T Min(T left, T right)
{
    return (left < right) ? left : right;
}

template <typename T>
T Sqr(T value)
{
    return value * value;
}

template <typename T>
T Abs(T value)
{
    return std::abs(value);
}

template <typename T>
T Clamp(T minValue, T maxValue, T value)
{
    return Max(Min(maxValue, value), minValue);
}

template <typename T>
T Saturate(T value)
{
	return Clamp(T(0), T(1), value);
}

template <typename T>
bool IsEqual(T left, T right, T epsilon)
{
    return (Abs(left - right) < epsilon);
}

template <typename T>
T Rcp(T value)
{
    return T(1) / value;
}

f32 ToDegrees(f32 radians);
f32 ToRadians(f32 degrees);

f32 Sin(f32 angleInRadians);
f32 ArcSin(f32 sinAngle);

f32 Cos(f32 angleInRadians);
f32 ArcCos(f32 cosAngle);

f32 Tan(f32 angleInRadians);
f32 ArcTan(f32 tanAngle);

void SinCos(f32& sinAngle, f32& cosAngle, f32 angleInRadians);

f32 Ceil(f32 value);
f64 Ceil(f64 value);

f32 Floor(f32 value);
f64 Floor(f64 value);

f32 Pow(f32 base, f32 exponent);
f64 Pow(f64 base, f64 exponent);

f32 Sqrt(f32 value);
f64 Sqrt(f64 value);

f32 Lerp(f32 minValue, f32 maxValue, f32 weight);
f64 Lerp(f64 minValue, f64 maxValue, f64 weight);

f32 SmoothStep(f32 minValue, f32 maxValue, f32 value);
f64 SmoothStep(f64 minValue, f64 maxValue, f64 value);

f32 SmootherStep(f32 minValue, f32 maxValue, f32 value);
f64 SmootherStep(f64 minValue, f64 maxValue, f64 value);

f32 SmoothestStep(f32 minValue, f32 maxValue, f32 value);
f64 SmoothestStep(f64 minValue, f64 maxValue, f64 value);