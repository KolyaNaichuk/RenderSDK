#pragma once

#include <cmath>
#include "Common/Common.h"

static const f32 PI = 3.141592653589793f;
static const f32 TWO_PI = 6.28318530717958647692f;
static const f32 RCP_PI = 0.31830988618379067154f;
static const f32 RCP_TWO_PI = 0.15915494309189533577f;
static const f32 PI_DIV_TWO = 1.57079632679489661923f;
static const f32 PI_DIV_FOUR = 0.78539816339744830962f;

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
T Sqrt(T value)
{
    return std::sqrt(value);
}

template <typename T>
T Sin(T angleInRadians)
{
    return std::sin(angleInRadians);
}

template <typename T>
T Cos(T angleInRadians)
{
    return std::cos(angleInRadians);
}

template <typename T> 
T Tan(T angleInRadians)
{
    return (T)std::tan((double)angleInRadians);
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
T Clamp(T lowerBound, T upperBound, T value)
{
    return Max(Min(upperBound, value), lowerBound);
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

f32 RadiansToDegrees(f32 radians);
f32 DegreesToRadians(f32 degrees);
