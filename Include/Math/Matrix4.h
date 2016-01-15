#pragma once

#include "Math/Math.h"

struct Matrix4f
{
    explicit Matrix4f();
    explicit Matrix4f(f32 scalar);
	explicit Matrix4f(f32 scalar00, f32 scalar01, f32 scalar02, f32 scalar03,
					  f32 scalar10, f32 scalar11, f32 scalar12, f32 scalar13,
					  f32 scalar20, f32 scalar21, f32 scalar22, f32 scalar23,
					  f32 scalar30, f32 scalar31, f32 scalar32, f32 scalar33);

	const Matrix4f operator- () const;
    
    f32 m_00, m_01, m_02, m_03;
    f32 m_10, m_11, m_12, m_13;
    f32 m_20, m_21, m_22, m_23;
    f32 m_30, m_31, m_32, m_33;

	static const Matrix4f IDENTITY;
	static const Matrix4f ZERO;
};

Matrix4f& operator+= (Matrix4f& matrix1, const Matrix4f& matrix2);
Matrix4f& operator-= (Matrix4f& matrix1, const Matrix4f& matrix2);
Matrix4f& operator*= (Matrix4f& matrix1, const Matrix4f& matrix2);

Matrix4f& operator+= (Matrix4f& matrix, f32 scalar);
Matrix4f& operator-= (Matrix4f& matrix, f32 scalar);
Matrix4f& operator*= (Matrix4f& matrix, f32 scalar);

const Matrix4f operator+ (const Matrix4f& matrix1, const Matrix4f& matrix2);
const Matrix4f operator- (const Matrix4f& matrix1, const Matrix4f& matrix2);
const Matrix4f operator* (const Matrix4f& matrix1, const Matrix4f& matrix2);

const Matrix4f operator+ (const Matrix4f& matrix, f32 scalar);
const Matrix4f operator+ (f32 scalar, const Matrix4f& matrix);

const Matrix4f operator- (const Matrix4f& matrix, f32 scalar);
const Matrix4f operator- (f32 scalar, const Matrix4f& matrix);

const Matrix4f operator* (const Matrix4f& matrix, f32 scalar);
const Matrix4f operator* (f32 scalar, const Matrix4f& matrix);

const Matrix4f Transpose(const Matrix4f& matrix);
f32 Determinant(const Matrix4f& matrix);
const Matrix4f Adjoint(const Matrix4f& matrix);
const Matrix4f Inverse(const Matrix4f& matrix);