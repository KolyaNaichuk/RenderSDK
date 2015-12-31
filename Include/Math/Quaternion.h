#pragma once

#include "Math/Math.h"
#include "Math/BasisAxes.h"

class Radian;
struct Vector3f;
struct Matrix4f;
struct AxisAngle;
struct EulerAngles;

struct Quaternion
{
	Quaternion();
	Quaternion(f32 w);
	Quaternion(f32 x, f32 y, f32 z);
	Quaternion(f32 x, f32 y, f32 z, f32 w);
	Quaternion(const AxisAngle& axisAngle);
	Quaternion(const EulerAngles& eulerAngles);
	Quaternion(const Matrix4f& rotationMatrix);

	const Quaternion operator- () const;
	
	f32 m_X;
	f32 m_Y;
	f32 m_Z;
	f32 m_W;
};

const BasisAxes ExtractBasisAxes(const Quaternion& quat);
const AxisAngle ExtractAxisAngle(const Quaternion& quat);

const Quaternion CreateRotationXQuaternion(const Radian& angle);
const Quaternion CreateRotationYQuaternion(const Radian& angle);
const Quaternion CreateRotationZQuaternion(const Radian& angle);

bool IsEqual(const Quaternion& quat1, const Quaternion& quat2, f32 epsilon = EPSILON);
const Quaternion Conjugate(const Quaternion& quat);
const Quaternion Normalize(const Quaternion& quat);
const Quaternion Inverse(const Quaternion& quat);
f32 Magnitude(const Quaternion& quat);
f32 MagnitudeSquared(const Quaternion& quat);
f32 Dot(const Quaternion& quat1, const Quaternion& quat2);

Quaternion& operator*= (Quaternion& quat, f32 scalar);
const Quaternion operator* (const Quaternion& quat, f32 scalar);

Quaternion& operator*= (Quaternion& quat1, const Quaternion& quat2);
const Quaternion operator* (const Quaternion& quat1, const Quaternion& quat2);

Quaternion& operator+= (Quaternion& quat1, const Quaternion& quat2);
const Quaternion operator+ (const Quaternion& quat1, const Quaternion& quat2);

Quaternion& operator-= (Quaternion& quat1, const Quaternion& quat2);
const Quaternion operator- (const Quaternion& quat1, const Quaternion& quat2);