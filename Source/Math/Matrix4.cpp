#include "Math/Matrix4.h"

#define DETERMINANT2X2(m00, m01, m10, m11) \
    (m00 * m11 - m01 * m10)

#define DETERMINANT3X3(m00, m01, m02, m10, m11, m12, m20, m21, m22) \
	(m20 * (m01 * m12 - m02 * m11) + \
     m21 * (m02 * m10 - m00 * m12) + \
     m22 * (m00 * m11 - m01 * m10))

const Matrix4f Matrix4f::IDENTITY(1.0f, 0.0f, 0.0f, 0.0f,
								  0.0f, 1.0f, 0.0f, 0.0f,
								  0.0f, 0.0f, 1.0f, 0.0f,
								  0.0f, 0.0f, 0.0f, 1.0f);

const Matrix4f Matrix4f::ZERO(0.0f, 0.0f, 0.0f, 0.0f,
							  0.0f, 0.0f, 0.0f, 0.0f,
							  0.0f, 0.0f, 0.0f, 0.0f,
							  0.0f, 0.0f, 0.0f, 0.0f);

Matrix4f::Matrix4f()
    : m_00(0.0f), m_01(0.0f), m_02(0.0f), m_03(0.0f)
    , m_10(0.0f), m_11(0.0f), m_12(0.0f), m_13(0.0f)
    , m_20(0.0f), m_21(0.0f), m_22(0.0f), m_23(0.0f)
    , m_30(0.0f), m_31(0.0f), m_32(0.0f), m_33(0.0f)
{
}

Matrix4f::Matrix4f(f32 scalar)
    : m_00(scalar), m_01(scalar), m_02(scalar), m_03(scalar)
    , m_10(scalar), m_11(scalar), m_12(scalar), m_13(scalar)
    , m_20(scalar), m_21(scalar), m_22(scalar), m_23(scalar)
    , m_30(scalar), m_31(scalar), m_32(scalar), m_33(scalar)
{
}

Matrix4f::Matrix4f(f32 scalar00, f32 scalar01, f32 scalar02, f32 scalar03,
                   f32 scalar10, f32 scalar11, f32 scalar12, f32 scalar13,
                   f32 scalar20, f32 scalar21, f32 scalar22, f32 scalar23,
                   f32 scalar30, f32 scalar31, f32 scalar32, f32 scalar33)
    : m_00(scalar00), m_01(scalar01), m_02(scalar02), m_03(scalar03)
    , m_10(scalar10), m_11(scalar11), m_12(scalar12), m_13(scalar13)
    , m_20(scalar20), m_21(scalar21), m_22(scalar22), m_23(scalar23)
    , m_30(scalar30), m_31(scalar31), m_32(scalar32), m_33(scalar33)
{
}

Matrix4f& operator+= (Matrix4f& left, const Matrix4f& right)
{
    left.m_00 += right.m_00;
    left.m_01 += right.m_01;
    left.m_02 += right.m_02;
    left.m_03 += right.m_03;

    left.m_10 += right.m_10;
    left.m_11 += right.m_11;
    left.m_12 += right.m_12;
    left.m_13 += right.m_13;

    left.m_20 += right.m_20;
    left.m_21 += right.m_21;
    left.m_22 += right.m_22;
    left.m_23 += right.m_23;

    left.m_30 += right.m_30;
    left.m_31 += right.m_31;
    left.m_32 += right.m_32;
    left.m_33 += right.m_33;

    return left;
}

Matrix4f& operator-= (Matrix4f& left, const Matrix4f& right)
{
    left.m_00 -= right.m_00;
    left.m_01 -= right.m_01;
    left.m_02 -= right.m_02;
    left.m_03 -= right.m_03;

    left.m_10 -= right.m_10;
    left.m_11 -= right.m_11;
    left.m_12 -= right.m_12;
    left.m_13 -= right.m_13;

    left.m_20 -= right.m_20;
    left.m_21 -= right.m_21;
    left.m_22 -= right.m_22;
    left.m_23 -= right.m_23;

    left.m_30 -= right.m_30;
    left.m_31 -= right.m_31;
    left.m_32 -= right.m_32;
    left.m_33 -= right.m_33;

    return left;
}

Matrix4f& operator*= (Matrix4f& left, const Matrix4f& right)
{
    f32 m_00 = left.m_00 * right.m_00 + left.m_01 * right.m_10 + left.m_02 * right.m_20 + left.m_03 * right.m_30;
    f32 m_01 = left.m_00 * right.m_01 + left.m_01 * right.m_11 + left.m_02 * right.m_21 + left.m_03 * right.m_31;
    f32 m_02 = left.m_00 * right.m_02 + left.m_01 * right.m_12 + left.m_02 * right.m_22 + left.m_03 * right.m_32;
    f32 m_03 = left.m_00 * right.m_03 + left.m_01 * right.m_13 + left.m_02 * right.m_23 + left.m_03 * right.m_33;
        
    f32 m_10 = left.m_10 * right.m_00 + left.m_11 * right.m_10 + left.m_12 * right.m_20 + left.m_13 * right.m_30;
    f32 m_11 = left.m_10 * right.m_01 + left.m_11 * right.m_11 + left.m_12 * right.m_21 + left.m_13 * right.m_31;
    f32 m_12 = left.m_10 * right.m_02 + left.m_11 * right.m_12 + left.m_12 * right.m_22 + left.m_13 * right.m_32;
    f32 m_13 = left.m_10 * right.m_03 + left.m_11 * right.m_13 + left.m_12 * right.m_23 + left.m_13 * right.m_33;

    f32 m_20 = left.m_20 * right.m_00 + left.m_21 * right.m_10 + left.m_22 * right.m_20 + left.m_23 * right.m_30;
    f32 m_21 = left.m_20 * right.m_01 + left.m_21 * right.m_11 + left.m_22 * right.m_21 + left.m_23 * right.m_31;
    f32 m_22 = left.m_20 * right.m_02 + left.m_21 * right.m_12 + left.m_22 * right.m_22 + left.m_23 * right.m_32;
    f32 m_23 = left.m_20 * right.m_03 + left.m_21 * right.m_13 + left.m_22 * right.m_23 + left.m_23 * right.m_33;

    f32 m_30 = left.m_30 * right.m_00 + left.m_31 * right.m_10 + left.m_32 * right.m_20 + left.m_33 * right.m_30;
    f32 m_31 = left.m_30 * right.m_01 + left.m_31 * right.m_11 + left.m_32 * right.m_21 + left.m_33 * right.m_31;
    f32 m_32 = left.m_30 * right.m_02 + left.m_31 * right.m_12 + left.m_32 * right.m_22 + left.m_33 * right.m_32;
    f32 m_33 = left.m_30 * right.m_03 + left.m_31 * right.m_13 + left.m_32 * right.m_23 + left.m_33 * right.m_33;

    left.m_00 = m_00;
    left.m_01 = m_01;
    left.m_02 = m_02;
    left.m_03 = m_03;

    left.m_10 = m_10;
    left.m_11 = m_11;
    left.m_12 = m_12;
    left.m_13 = m_13;

    left.m_20 = m_20;
    left.m_21 = m_21;
    left.m_22 = m_22;
    left.m_23 = m_23;

    left.m_30 = m_30;
    left.m_31 = m_31;
    left.m_32 = m_32;
    left.m_33 = m_33;

    return left;
}

Matrix4f& operator+= (Matrix4f& left, f32 scalar)
{
    left.m_00 += scalar;
    left.m_01 += scalar;
    left.m_02 += scalar;
    left.m_03 += scalar;

    left.m_10 += scalar;
    left.m_11 += scalar;
    left.m_12 += scalar;
    left.m_13 += scalar;

    left.m_20 += scalar;
    left.m_21 += scalar;
    left.m_22 += scalar;
    left.m_23 += scalar;

    left.m_30 += scalar;
    left.m_31 += scalar;
    left.m_32 += scalar;
    left.m_33 += scalar;

    return left;
}

Matrix4f& operator-= (Matrix4f& left, f32 scalar)
{
    left.m_00 -= scalar;
    left.m_01 -= scalar;
    left.m_02 -= scalar;
    left.m_03 -= scalar;

    left.m_10 -= scalar;
    left.m_11 -= scalar;
    left.m_12 -= scalar;
    left.m_13 -= scalar;

    left.m_20 -= scalar;
    left.m_21 -= scalar;
    left.m_22 -= scalar;
    left.m_23 -= scalar;

    left.m_30 -= scalar;
    left.m_31 -= scalar;
    left.m_32 -= scalar;
    left.m_33 -= scalar;

    return left;
}

Matrix4f& operator*= (Matrix4f& left, f32 scalar)
{
    left.m_00 *= scalar;
    left.m_01 *= scalar;
    left.m_02 *= scalar;
    left.m_03 *= scalar;

    left.m_10 *= scalar;
    left.m_11 *= scalar;
    left.m_12 *= scalar;
    left.m_13 *= scalar;

    left.m_20 *= scalar;
    left.m_21 *= scalar;
    left.m_22 *= scalar;
    left.m_23 *= scalar;

    left.m_30 *= scalar;
    left.m_31 *= scalar;
    left.m_32 *= scalar;
    left.m_33 *= scalar;

    return left;
}

const Matrix4f operator+ (const Matrix4f& left, const Matrix4f& right)
{
    return (Matrix4f(left) += right);
}

const Matrix4f operator- (const Matrix4f& left, const Matrix4f& right)
{
    return (Matrix4f(left) -= right);
}

const Matrix4f operator* (const Matrix4f& left, const Matrix4f& right)
{
    return (Matrix4f(left) *= right);
}

const Matrix4f operator+ (const Matrix4f& left, f32 scalar)
{
    return (Matrix4f(left) += scalar);
}

const Matrix4f operator+ (f32 scalar, const Matrix4f& right)
{
	return (right + scalar);
}

const Matrix4f operator- (const Matrix4f& left, f32 scalar)
{
    return (Matrix4f(left) -= scalar);
}

const Matrix4f operator- (f32 scalar, const Matrix4f& right)
{
	return (Matrix4f(scalar) -= right);
}

const Matrix4f operator* (const Matrix4f& left, f32 scalar)
{
    return (Matrix4f(left) *= scalar);
}

const Matrix4f operator* (f32 scalar, const Matrix4f& right)
{
	return (right * scalar);
}

const Matrix4f Transpose(const Matrix4f& matrix)
{
    return Matrix4f(matrix.m_00, matrix.m_10, matrix.m_20, matrix.m_30,
                    matrix.m_01, matrix.m_11, matrix.m_21, matrix.m_31,
                    matrix.m_02, matrix.m_12, matrix.m_22, matrix.m_32,
                    matrix.m_03, matrix.m_13, matrix.m_23, matrix.m_33);
}

f32 Determinant(const Matrix4f& matrix)
{
	f32 cofactor00 = DETERMINANT3X3(matrix.m_11, matrix.m_12, matrix.m_13,
		matrix.m_21, matrix.m_22, matrix.m_23,
		matrix.m_31, matrix.m_32, matrix.m_33);

	f32 cofactor01 = -DETERMINANT3X3(matrix.m_10, matrix.m_12, matrix.m_13,
		matrix.m_20, matrix.m_22, matrix.m_23,
		matrix.m_30, matrix.m_32, matrix.m_33);

	f32 cofactor02 = DETERMINANT3X3(matrix.m_10, matrix.m_11, matrix.m_13,
		matrix.m_20, matrix.m_21, matrix.m_23,
		matrix.m_30, matrix.m_31, matrix.m_33);

	f32 cofactor03 = -DETERMINANT3X3(matrix.m_10, matrix.m_11, matrix.m_12,
		matrix.m_20, matrix.m_21, matrix.m_22,
		matrix.m_30, matrix.m_31, matrix.m_32);

    return (matrix.m_00 * cofactor00
		  + matrix.m_01 * cofactor01
		  + matrix.m_02 * cofactor02
		  + matrix.m_03 * cofactor03);
}

const Matrix4f Adjoint(const Matrix4f& matrix)
{
	f32 cofactor00 = DETERMINANT3X3(matrix.m_11, matrix.m_12, matrix.m_13,
		matrix.m_21, matrix.m_22, matrix.m_23,
		matrix.m_31, matrix.m_32, matrix.m_33);

	f32 cofactor01 = -DETERMINANT3X3(matrix.m_10, matrix.m_12, matrix.m_13,
		matrix.m_20, matrix.m_22, matrix.m_23,
		matrix.m_30, matrix.m_32, matrix.m_33);

	f32 cofactor02 = DETERMINANT3X3(matrix.m_10, matrix.m_11, matrix.m_13,
		matrix.m_20, matrix.m_21, matrix.m_23,
		matrix.m_30, matrix.m_31, matrix.m_33);

	f32 cofactor03 = -DETERMINANT3X3(matrix.m_10, matrix.m_11, matrix.m_12,
		matrix.m_20, matrix.m_21, matrix.m_22,
		matrix.m_30, matrix.m_31, matrix.m_32);

	f32 cofactor10 = -DETERMINANT3X3(matrix.m_01, matrix.m_02, matrix.m_03,
		matrix.m_21, matrix.m_22, matrix.m_23,
		matrix.m_31, matrix.m_32, matrix.m_33);

	f32 cofactor11 = DETERMINANT3X3(matrix.m_00, matrix.m_02, matrix.m_03,
		matrix.m_20, matrix.m_22, matrix.m_23,
		matrix.m_30, matrix.m_32, matrix.m_33);

	f32 cofactor12 = -DETERMINANT3X3(matrix.m_00, matrix.m_01, matrix.m_03,
		matrix.m_20, matrix.m_21, matrix.m_23,
		matrix.m_30, matrix.m_31, matrix.m_33);

	f32 cofactor13 = DETERMINANT3X3(matrix.m_00, matrix.m_01, matrix.m_02,
		matrix.m_20, matrix.m_21, matrix.m_22,
		matrix.m_30, matrix.m_31, matrix.m_32);

	f32 cofactor20 = DETERMINANT3X3(matrix.m_01, matrix.m_02, matrix.m_03,
		matrix.m_11, matrix.m_12, matrix.m_13,
		matrix.m_31, matrix.m_32, matrix.m_33);

	f32 cofactor21 = -DETERMINANT3X3(matrix.m_00, matrix.m_02, matrix.m_03,
		matrix.m_10, matrix.m_12, matrix.m_13,
		matrix.m_30, matrix.m_32, matrix.m_33);

	f32 cofactor22 = DETERMINANT3X3(matrix.m_00, matrix.m_01, matrix.m_03,
		matrix.m_10, matrix.m_11, matrix.m_13,
		matrix.m_30, matrix.m_31, matrix.m_33);

	f32 cofactor23 = -DETERMINANT3X3(matrix.m_00, matrix.m_01, matrix.m_02,
		matrix.m_10, matrix.m_11, matrix.m_12,
		matrix.m_30, matrix.m_31, matrix.m_32);

	f32 cofactor30 = -DETERMINANT3X3(matrix.m_01, matrix.m_02, matrix.m_03,
		matrix.m_11, matrix.m_12, matrix.m_13,
		matrix.m_21, matrix.m_22, matrix.m_23);

	f32 cofactor31 = DETERMINANT3X3(matrix.m_00, matrix.m_02, matrix.m_03,
		matrix.m_10, matrix.m_12, matrix.m_13,
		matrix.m_20, matrix.m_22, matrix.m_23);

	f32 cofactor32 = -DETERMINANT3X3(matrix.m_00, matrix.m_01, matrix.m_03,
		matrix.m_10, matrix.m_11, matrix.m_13,
		matrix.m_20, matrix.m_21, matrix.m_23);

	f32 cofactor33 = DETERMINANT3X3(matrix.m_00, matrix.m_01, matrix.m_02,
		matrix.m_10, matrix.m_11, matrix.m_12,
		matrix.m_20, matrix.m_21, matrix.m_22);

	return Matrix4f(cofactor00, cofactor10, cofactor20, cofactor30,
					cofactor01, cofactor11, cofactor21, cofactor31,
					cofactor02, cofactor12, cofactor22, cofactor32,
					cofactor03, cofactor13, cofactor23, cofactor33);
}

const Matrix4f Inverse(const Matrix4f& matrix)
{
	f32 det = Determinant(matrix);
	assert(det > EPSILON);

    return Rcp(det) * Adjoint(matrix);
}
