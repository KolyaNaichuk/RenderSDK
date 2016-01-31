#include "Math/SphericalHarmonics.h"
#include "Math/Radian.h"

namespace
{
	f32 K(i32 l, i32 m);
	i32 Factorial(i32 number);
	i32 DoubleFactorial(i32 number);
	f32 Legendre(i32 l, i32 m, f32 x);
}

f32 SphericalHarmonic(i32 l, i32 m, const Radian& theta, const Radian& phi)
{
	if (m > 0)
		return Sqrt(2.0f) * K(l, m) * Cos(f32(m) * phi) * Legendre(l, m, Cos(theta));

	if (m < 0)
		return Sqrt(2.0f) * K(l, m) * Sin(f32(-m) * phi) * Legendre(l, -m, Cos(theta));

	return K(l, m) * Legendre(l, m, Cos(theta));
}

namespace
{
	f32 K(i32 l, i32 m)
	{
		f32 num = f32((2 * l + 1) * Factorial(l - Abs(m)));
		f32 denom = 4.0f * PI * f32(Factorial(l + Abs(m)));

		return Sqrt(num / denom);
	}

	i32 Factorial(i32 number)
	{
		if (number > 0)
			return number * Factorial(number - 1);
		return 1;
	}

	i32 DoubleFactorial(i32 number)
	{
		if (number > 1)
			return number * DoubleFactorial(number - 2);
		return 1;
	}

	f32 Legendre(i32 l, i32 m, f32 x)
	{
		if (l == m)
			return Pow(-1.0f, f32(m)) * f32(DoubleFactorial(2 * m - 1)) * Pow(1.0f - x * x, 0.5f * f32(m));

		if (l == m + 1)
			return x * f32(2 * m + 1) * Legendre(m, m, x);

		return (x * f32(2 * l - 1) * Legendre(l - 1, m, x) - f32(l + m - 1) * Legendre(l - 2, m, x)) / f32(l - m);
	}
}
