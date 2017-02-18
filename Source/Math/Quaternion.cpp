#include "Math/Quaternion.h"
#include "Math/AxisAngle.h"
#include "Math/EulerAngles.h"
#include "Math/Vector3.h"
#include "Math/Matrix4.h"

Quaternion::Quaternion()
	: Quaternion(0.0f, 0.0f, 0.0f, 1.0f)
{
}

Quaternion::Quaternion(f32 w)
	: Quaternion(0.0f, 0.0f, 0.0f, w)
{
}

Quaternion::Quaternion(f32 x, f32 y, f32 z)
	: Quaternion(x, y, z, 0.0f)
{
}

Quaternion::Quaternion(f32 x, f32 y, f32 z, f32 w)
	: m_X(x)
	, m_Y(y)
	, m_Z(z)
	, m_W(w)
{
}

Quaternion::Quaternion(const AxisAngle& axisAngle)
{
	assert(IsNormalized(axisAngle.m_Axis));
	
	f32 sinHalfAngle, cosHalfAngle;
	SinCos(sinHalfAngle, cosHalfAngle, 0.5f * axisAngle.m_AngleInRadians);

	m_W = cosHalfAngle;
	m_X = sinHalfAngle * axisAngle.m_Axis.m_X;
	m_Y = sinHalfAngle * axisAngle.m_Axis.m_Y;
	m_Z = sinHalfAngle * axisAngle.m_Axis.m_Z;
}

Quaternion::Quaternion(const EulerAngles& eulerAngles)
{
	f32 sinAngleZ, cosAngleZ;
	SinCos(sinAngleZ, cosAngleZ, 0.5f * eulerAngles.m_ZAxisAngleInRadians);

	f32 sinAngleX, cosAngleX;
	SinCos(sinAngleX, cosAngleX, 0.5f * eulerAngles.m_XAxisAngleInRadians);

	f32 sinAngleY, cosAngleY;
	SinCos(sinAngleY, cosAngleY, 0.5f * eulerAngles.m_YAxisAngleInRadians);

	m_W = cosAngleX * cosAngleY * cosAngleZ + sinAngleX * sinAngleY * sinAngleZ;
	m_X = sinAngleX * cosAngleY * cosAngleZ + cosAngleX * sinAngleY * sinAngleZ;
	m_Y = cosAngleX * sinAngleY * cosAngleZ - sinAngleX * cosAngleY * sinAngleZ;
	m_Z = cosAngleX * cosAngleY * sinAngleZ - sinAngleX * sinAngleY * cosAngleZ;
}

Quaternion::Quaternion(const Matrix4f& rotationMatrix)
{
	enum { kWComp = 0, kXComp, kYComp, kZComp, kNumComps };

	f32 mul4SqrCompMinus1[kNumComps];
	mul4SqrCompMinus1[kWComp] = rotationMatrix.m_00 + rotationMatrix.m_11 + rotationMatrix.m_22;
	mul4SqrCompMinus1[kXComp] = rotationMatrix.m_00 - rotationMatrix.m_11 - rotationMatrix.m_22;
	mul4SqrCompMinus1[kYComp] = rotationMatrix.m_11 - rotationMatrix.m_00 - rotationMatrix.m_22;
	mul4SqrCompMinus1[kZComp] = rotationMatrix.m_22 - rotationMatrix.m_00 - rotationMatrix.m_11;

	u8  maxProdComp = kWComp;
	if (mul4SqrCompMinus1[maxProdComp] < mul4SqrCompMinus1[kXComp])
		maxProdComp = kXComp;
	if (mul4SqrCompMinus1[maxProdComp] < mul4SqrCompMinus1[kYComp])
		maxProdComp = kYComp;
	if (mul4SqrCompMinus1[maxProdComp] < mul4SqrCompMinus1[kZComp])
		maxProdComp = kZComp;
	
	switch (maxProdComp)
	{
		case kWComp:
		{
			m_W = 0.5f * Sqrt(1.0f + mul4SqrCompMinus1[kWComp]);
			f32 rcp4MulW = 0.25f / m_W;
			
			m_X = rcp4MulW * (rotationMatrix.m_12 - rotationMatrix.m_21);
			m_Y = rcp4MulW * (rotationMatrix.m_20 - rotationMatrix.m_02);
			m_Z = rcp4MulW * (rotationMatrix.m_01 - rotationMatrix.m_10);

			break;
		}
		case kXComp:
		{
			m_X = 0.5f * Sqrt(1.0f + mul4SqrCompMinus1[kXComp]);
			f32 rcp4MulX = 0.25f / m_X;

			m_W = rcp4MulX * (rotationMatrix.m_12 - rotationMatrix.m_21);
			m_Y = rcp4MulX * (rotationMatrix.m_01 + rotationMatrix.m_10);
			m_Z = rcp4MulX * (rotationMatrix.m_20 + rotationMatrix.m_02);
			break;
		}
		case kYComp:
		{
			m_Y = 0.5f * Sqrt(1.0f + mul4SqrCompMinus1[kYComp]);
			f32 rcp4MulY = 0.25f / m_Y;

			m_W = rcp4MulY * (rotationMatrix.m_20 - rotationMatrix.m_02);
			m_X = rcp4MulY * (rotationMatrix.m_01 + rotationMatrix.m_10);
			m_Z = rcp4MulY * (rotationMatrix.m_12 + rotationMatrix.m_21);

			break;
		}
		case kZComp:
		{
			m_Z = 0.5f * Sqrt(1.0f + mul4SqrCompMinus1[kZComp]);
			f32 rcp4MulZ = 0.25f / m_Z;

			m_W = rcp4MulZ * (rotationMatrix.m_01 - rotationMatrix.m_10);
			m_X = rcp4MulZ * (rotationMatrix.m_20 + rotationMatrix.m_02);
			m_Y = rcp4MulZ * (rotationMatrix.m_12 + rotationMatrix.m_21);

			break;
		}
	}
}

const Quaternion Quaternion::operator- () const
{
	return Quaternion(-m_X, -m_Y, -m_Z, -m_W);
}

const BasisAxes ExtractBasisAxes(const Quaternion& quat)
{
	f32 x2 = 2.0f * quat.m_X;
	f32 y2 = 2.0f * quat.m_Y;
	f32 z2 = 2.0f * quat.m_Z;

	f32 xx2 = x2 * quat.m_X;
	f32 yy2 = y2 * quat.m_Y;
	f32 zz2 = z2 * quat.m_Z;

	f32 xy2 = x2 * quat.m_Y;
	f32 xz2 = x2 * quat.m_Z;
	f32 xw2 = x2 * quat.m_W;
	f32 yz2 = y2 * quat.m_Z;
	f32 yw2 = y2 * quat.m_W;
	f32 zw2 = z2 * quat.m_W;

	Vector3f xAxis(1.0f - yy2 - zz2, xy2 + zw2, xz2 - yw2);
	Vector3f yAxis(xy2 - zw2, 1.0f - xx2 - zz2, yz2 + xw2);
	Vector3f zAxis(xz2 + yw2, yz2 - xw2, 1.0f - xx2 - yy2);

	return BasisAxes(xAxis, yAxis, zAxis);
}

const AxisAngle ExtractAxisAngle(const Quaternion& quat)
{
	f32 sqrSinHalfAngle = quat.m_X * quat.m_X + quat.m_Y * quat.m_Y + quat.m_Z * quat.m_Z;
	if (sqrSinHalfAngle > EPSILON)
	{
		f32 rcpSinHalfAngle = 1.0f / Sqrt(sqrSinHalfAngle);
		
		f32 angleInRadians = 2.0f * ArcCos(quat.m_W);
		Vector3f axis = rcpSinHalfAngle * Vector3f(quat.m_X, quat.m_Y, quat.m_Z);

		return AxisAngle(axis, angleInRadians);
	}
	return AxisAngle(Vector3f(1.0f, 0.0f, 0.0f), 0.0f);
}

const Quaternion CreateRotationXQuaternion(f32 angleInRadians)
{
	f32 sinHalfAngle, cosHalfAngle;
	SinCos(sinHalfAngle, cosHalfAngle, 0.5f * angleInRadians);

	return Quaternion(sinHalfAngle, 0.0f, 0.0f, cosHalfAngle);
}

const Quaternion CreateRotationYQuaternion(f32 angleInRadians)
{
	f32 sinHalfAngle, cosHalfAngle;
	SinCos(sinHalfAngle, cosHalfAngle, 0.5f * angleInRadians);

	return Quaternion(0.0f, sinHalfAngle, 0.0f, cosHalfAngle);
}

const Quaternion CreateRotationZQuaternion(f32 angleInRadians)
{
	f32 sinHalfAngle, cosHalfAngle;
	SinCos(sinHalfAngle, cosHalfAngle, 0.5f * angleInRadians);

	return Quaternion(0.0f, 0.0f, sinHalfAngle, cosHalfAngle);
}

bool IsEqual(const Quaternion& quat1, const Quaternion& quat2, f32 epsilon)
{
	return ::IsEqual(quat1.m_X, quat2.m_X, epsilon) &&
		::IsEqual(quat1.m_Y, quat2.m_Y, epsilon) &&
		::IsEqual(quat1.m_Z, quat2.m_Z, epsilon) &&
		::IsEqual(quat1.m_W, quat2.m_W, epsilon);
}

const Quaternion Conjugate(const Quaternion& quat)
{
	return Quaternion(-quat.m_X, -quat.m_Y, -quat.m_Z, quat.m_W);
}

const Quaternion Normalize(const Quaternion& quat)
{
	return (Rcp(Magnitude(quat)) * quat);
}

const Quaternion Inverse(const Quaternion& quat)
{
	return (Rcp(MagnitudeSquared(quat)) * Conjugate(quat));
}

f32 Magnitude(const Quaternion& quat)
{
	return Sqrt(MagnitudeSquared(quat));
}

f32 MagnitudeSquared(const Quaternion& quat)
{
	return Dot(quat, quat);
}

f32 Dot(const Quaternion& quat1, const Quaternion& quat2)
{
	return (quat1.m_W * quat2.m_W + quat1.m_X * quat2.m_X + quat1.m_Y * quat2.m_Y + quat1.m_Z * quat2.m_Z);
}

Quaternion& operator*= (Quaternion& quat, f32 scalar)
{
	quat.m_X *= scalar;
	quat.m_Y *= scalar;
	quat.m_Z *= scalar;
	quat.m_W *= scalar;
	
	return quat;
}

const Quaternion operator* (const Quaternion& quat, f32 scalar)
{
	return (Quaternion(quat) *= scalar);
}

const Quaternion operator* (f32 scalar, const Quaternion& quat)
{
	return (Quaternion(quat) *= scalar);
}

Quaternion& operator*= (Quaternion& quat1, const Quaternion& quat2)
{
	f32 x = quat1.m_W * quat2.m_X + quat1.m_X * quat2.m_W + quat1.m_Y * quat2.m_Z - quat1.m_Z * quat2.m_Y;
	f32 y = quat1.m_W * quat2.m_Y + quat1.m_Y * quat2.m_W + quat1.m_Z * quat2.m_Y - quat1.m_X * quat2.m_Z;
	f32 z = quat1.m_W * quat2.m_Z + quat1.m_Z * quat2.m_W + quat1.m_X * quat2.m_Y - quat1.m_Y * quat2.m_X;

	quat1.m_W = quat1.m_W * quat2.m_W - quat1.m_X * quat2.m_X - quat1.m_Y * quat2.m_Y - quat1.m_Z * quat2.m_Z;
	quat1.m_X = x;
	quat1.m_Y = y;
	quat1.m_Z = z;

	return quat1;
}

const Quaternion operator* (const Quaternion& quat1, const Quaternion& quat2)
{
	return (Quaternion(quat1) *= quat2);
}

Quaternion& operator+= (Quaternion& quat1, const Quaternion& quat2)
{
	quat1.m_X += quat2.m_X;
	quat1.m_Y += quat2.m_Y;
	quat1.m_Z += quat2.m_Z;
	quat1.m_W += quat2.m_W;
	
	return quat1;
}

const Quaternion operator+ (const Quaternion& quat1, const Quaternion& quat2)
{
	return (Quaternion(quat1) += quat2);
}

Quaternion& operator-= (Quaternion& quat1, const Quaternion& quat2)
{
	quat1.m_X -= quat2.m_X;
	quat1.m_Y -= quat2.m_Y;
	quat1.m_Z -= quat2.m_Z;
	quat1.m_W -= quat2.m_W;
	
	return quat1;
}

const Quaternion operator- (const Quaternion& quat1, const Quaternion& quat2)
{
	return (Quaternion(quat1) -= quat2);
}