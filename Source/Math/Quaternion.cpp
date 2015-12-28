#include "Math/Quaternion.h"

Quaternion::Quaternion()
	: Quaternion(0.0f, 0.0f, 0.0f, 0.0f)
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

const Quaternion Quaternion::operator- () const
{
	return Quaternion(-m_X, -m_Y, -m_Z, -m_W);
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