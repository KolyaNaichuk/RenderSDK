#pragma once

#include "Math/Math.h"

struct Matrix3f
{
	explicit Matrix3f();
	explicit Matrix3f(f32 scalar);
	explicit Matrix3f(f32 scalar00, f32 scalar01, f32 scalar02,
					  f32 scalar10, f32 scalar11, f32 scalar12,
					  f32 scalar20, f32 scalar21, f32 scalar22);

	const Matrix3f operator- () const;

	f32 m_00, m_01, m_02;
	f32 m_10, m_11, m_12;
	f32 m_20, m_21, m_22;

	static const Matrix3f IDENTITY;
	static const Matrix3f ZERO;
};

const Matrix3f operator* (const Matrix3f& matrix, f32 scalar);
const Matrix3f operator* (f32 scalar, const Matrix3f& matrix);

const Matrix3f Transpose(const Matrix3f& matrix);
f32 Determinant(const Matrix3f& matrix);
const Matrix3f Adjoint(const Matrix3f& matrix);
const Matrix3f Inverse(const Matrix3f& matrix);