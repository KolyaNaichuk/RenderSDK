#include "Math/Math.h"
#include "Math/Radian.h"

f32 RadiansToDegrees(f32 radians)
{
	static const f32 radiansToDegrees = 180.0f / PI;
	return radiansToDegrees * radians;
}

f32 DegreesToRadians(f32 degrees)
{
	static const f32 degreesToRadians = PI / 180.0f;
	return degreesToRadians * degrees;
}

f32 Sin(const Radian& angle)
{
	return std::sinf(angle.Get());
}

f32 Cos(const Radian& angle)
{
	return std::cosf(angle.Get());
}

const Radian ACos(f32 cosAngle)
{
	return Radian(std::acosf(cosAngle));
}

f32 Tan(const Radian& angle)
{
	return (f32)std::tan((f64)angle.Get());
}

void SinCos(f32& sinAngle, f32& cosAngle, const Radian& angle)
{
	sinAngle = Sin(angle);
	cosAngle = Cos(angle);
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