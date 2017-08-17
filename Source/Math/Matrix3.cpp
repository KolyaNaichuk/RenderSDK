#include "Math/Matrix3.h"

#define DETERMINANT2X2(m00, m01, m10, m11) \
    (m00 * m11 - m01 * m10)

const Matrix3f Matrix3f::IDENTITY(1.0f, 0.0f, 0.0f,
								  0.0f, 1.0f, 0.0f,
	                              0.0f, 0.0f, 1.0f);

const Matrix3f Matrix3f::ZERO(0.0f, 0.0f, 0.0f,
							  0.0f, 0.0f, 0.0f,
							  0.0f, 0.0f, 0.0f);

Matrix3f::Matrix3f()
	: Matrix3f(0.0f)
{
}

Matrix3f::Matrix3f(f32 scalar)
	: Matrix3f(scalar, scalar, scalar,
			   scalar, scalar, scalar,
			   scalar, scalar, scalar)
{
}

Matrix3f::Matrix3f(f32 scalar00, f32 scalar01, f32 scalar02,
				   f32 scalar10, f32 scalar11, f32 scalar12,
				   f32 scalar20, f32 scalar21, f32 scalar22)
	: m_00(scalar00), m_01(scalar01), m_02(scalar02)
	, m_10(scalar10), m_11(scalar11), m_12(scalar12)
	, m_20(scalar20), m_21(scalar21), m_22(scalar22)
{
}

const Matrix3f Matrix3f::operator- () const
{
	return Matrix3f(-m_00, -m_01, -m_02,
					-m_10, -m_11, -m_12,
					-m_20, -m_21, -m_22);
}

const Matrix3f operator* (const Matrix3f& matrix, f32 scalar)
{
	return Matrix3f(scalar * matrix.m_00, scalar * matrix.m_01, scalar * matrix.m_02,
					scalar * matrix.m_10, scalar * matrix.m_11, scalar * matrix.m_12,
					scalar * matrix.m_20, scalar * matrix.m_21, scalar * matrix.m_22);
}

const Matrix3f operator* (f32 scalar, const Matrix3f& matrix)
{
	return Matrix3f(scalar * matrix.m_00, scalar * matrix.m_01, scalar * matrix.m_02,
					scalar * matrix.m_10, scalar * matrix.m_11, scalar * matrix.m_12,
					scalar * matrix.m_20, scalar * matrix.m_21, scalar * matrix.m_22);
}

const Matrix3f Transpose(const Matrix3f& matrix)
{
	return Matrix3f(matrix.m_00, matrix.m_10, matrix.m_20,
		            matrix.m_01, matrix.m_11, matrix.m_21,
					matrix.m_02, matrix.m_12, matrix.m_22);
}

f32 Determinant(const Matrix3f& matrix)
{
	return (matrix.m_20 * (matrix.m_01 * matrix.m_12 - matrix.m_02 * matrix.m_11) +
		    matrix.m_21 * (matrix.m_02 * matrix.m_10 - matrix.m_00 * matrix.m_12) +
		    matrix.m_22 * (matrix.m_00 * matrix.m_11 - matrix.m_01 * matrix.m_10));
}

const Matrix3f Adjoint(const Matrix3f& matrix)
{
	f32 cofactor00 =  DETERMINANT2X2(matrix.m_11, matrix.m_12, matrix.m_21, matrix.m_22);
	f32 cofactor01 = -DETERMINANT2X2(matrix.m_10, matrix.m_12, matrix.m_20, matrix.m_22);
	f32 cofactor02 =  DETERMINANT2X2(matrix.m_10, matrix.m_11, matrix.m_20, matrix.m_21);

	f32 cofactor10 = -DETERMINANT2X2(matrix.m_01, matrix.m_02, matrix.m_21, matrix.m_22);
	f32 cofactor11 =  DETERMINANT2X2(matrix.m_00, matrix.m_02, matrix.m_20, matrix.m_22);
	f32 cofactor12 = -DETERMINANT2X2(matrix.m_00, matrix.m_01, matrix.m_20, matrix.m_21);

	f32 cofactor20 =  DETERMINANT2X2(matrix.m_01, matrix.m_02, matrix.m_11, matrix.m_12);
	f32 cofactor21 = -DETERMINANT2X2(matrix.m_00, matrix.m_02, matrix.m_10, matrix.m_12);
	f32 cofactor22 =  DETERMINANT2X2(matrix.m_00, matrix.m_01, matrix.m_10, matrix.m_11);

	return Matrix3f(cofactor00, cofactor10, cofactor20,
					cofactor01, cofactor11, cofactor21,
					cofactor02, cofactor12, cofactor22);
}

const Matrix3f Inverse(const Matrix3f& matrix)
{
	f32 det = Determinant(matrix);
	assert(!AreEqual(det, 0.0f, EPSILON));

	return Rcp(det) * Adjoint(matrix);
}