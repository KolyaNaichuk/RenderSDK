#include "Math/SphericalHarmonics.h"
#include "Math/Radian.h"
#include "Math/Vector3.h"

namespace
{
	f32 K(i32 l, i32 m);
	i32 Factorial(i32 number);
	i32 DoubleFactorial(i32 number);
	f32 Legendre(i32 l, i32 m, f32 x);
}

f32 SHEvaluate(i32 l, i32 m, const Radian& theta, const Radian& phi)
{
	assert(l >= 0);
	assert(IsInRange(-l, l, m));

	if (m > 0)
		return Sqrt(2.0f) * K(l, m) * Cos(f32(m) * phi) * Legendre(l, m, Cos(theta));

	if (m < 0)
		return Sqrt(2.0f) * K(l, m) * Sin(f32(-m) * phi) * Legendre(l, -m, Cos(theta));

	return K(l, m) * Legendre(l, m, Cos(theta));
}

i32 SHGetNumBasisFuncs(i32 numBands)
{
	return numBands * numBands;
}

void SHEvaluateBasis(f32* pBasisFuncValues, i32 numBands, const Vector3f& dir)
{
	assert(IsNormalized(dir));
	assert(IsInRange(1, 5, numBands));
	
	pBasisFuncValues[0] = 0.282095f;
	if (numBands == 1)
		return;

	pBasisFuncValues[1] = -0.488603f * dir.m_Y;
	pBasisFuncValues[2] =  0.488603f * dir.m_Z;
	pBasisFuncValues[3] = -0.488603f * dir.m_X;
	if (numBands == 2)
		return;

	const Vector3f sqrDir = Sqr(dir);

	pBasisFuncValues[4] =  1.092548f * dir.m_X * dir.m_Y;
	pBasisFuncValues[5] = -1.092548f * dir.m_Y * dir.m_Z;
	pBasisFuncValues[6] =  0.315392f * (3.0f * sqrDir.m_Z - 1.0f);
	pBasisFuncValues[7] = -1.092548f * dir.m_X * dir.m_Z;
	pBasisFuncValues[8] =  0.546274f * (sqrDir.m_X - sqrDir.m_Y);
	if (numBands == 3)
		return;

	pBasisFuncValues[ 9] = -0.590044f * dir.m_Y * (3.0f * sqrDir.m_X - sqrDir.m_Y);
	pBasisFuncValues[10] =  2.890611f * dir.m_X * dir.m_Y * dir.m_Z;
	pBasisFuncValues[11] = -0.457046f * dir.m_Y * (5.0f * sqrDir.m_Z - 1.0f);
	pBasisFuncValues[12] =  0.373176f * dir.m_Z * (5.0f * sqrDir.m_Z - 3.0f);
	pBasisFuncValues[13] = -0.457046f * dir.m_X * (5.0f * sqrDir.m_Z - 1.0f);
	pBasisFuncValues[14] =  1.445306f * dir.m_Z * (sqrDir.m_X - sqrDir.m_Y);
	pBasisFuncValues[15] = -0.590044f * dir.m_X * (sqrDir.m_X - 3.0f * sqrDir.m_Y);
	if (numBands == 4)
		return;
	
	pBasisFuncValues[16] =  2.503343f * dir.m_X * dir.m_Y * (sqrDir.m_X - sqrDir.m_Y);
	pBasisFuncValues[17] = -1.770131f * dir.m_Y * dir.m_Z * (3.0f * sqrDir.m_X - sqrDir.m_Y);
	pBasisFuncValues[18] =  0.946175f * dir.m_X * dir.m_Y * (7.0f * sqrDir.m_Z - 1.0f);
	pBasisFuncValues[19] = -0.669047f * dir.m_Y * dir.m_Z * (7.0f * sqrDir.m_Z - 3.0f);
	pBasisFuncValues[20] =  0.105786f * (35.0f * Sqr(sqrDir.m_Z) - 30.0f * sqrDir.m_Z + 3.0f);
	pBasisFuncValues[21] = -0.669047f * dir.m_X * dir.m_Z * (7.0f * sqrDir.m_Z - 3.0f);
	pBasisFuncValues[22] =  0.473087f * (sqrDir.m_X - sqrDir.m_Y) * (7.0f * sqrDir.m_Z - 1.0f);
	pBasisFuncValues[23] = -1.770131f * dir.m_X * dir.m_Z * (sqrDir.m_X - 3.0f * sqrDir.m_Y);
	pBasisFuncValues[24] =  0.625836f * (Sqr(sqrDir.m_X) - 6.0f * sqrDir.m_X * sqrDir.m_Y + Sqr(sqrDir.m_Y));
	if (numBands == 5)
		return;
}

i32 SHGetBasisFuncIndex(i32 l, i32 m)
{
	return (l * (l + 1) + m);
}

void SHProjectClampedCosineFunc(f32* pCoeffs, i32 numBands, const Vector3f& direction)
{
	assert(false);

	pCoeffs[SHGetBasisFuncIndex(0,  0)] = 0.0f;
	if (numBands == 1)
		return;

	pCoeffs[SHGetBasisFuncIndex(1, -1)] = 0.0f;
	pCoeffs[SHGetBasisFuncIndex(1,  0)] = 0.0f;
	pCoeffs[SHGetBasisFuncIndex(1,  1)] = 0.0f;
	if (numBands == 2)
		return;
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
