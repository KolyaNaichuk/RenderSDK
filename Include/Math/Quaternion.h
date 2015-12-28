#pragma once

#include "Math/Math.h"

struct Quaternion
{
	Quaternion();
	Quaternion(f32 w);
	Quaternion(f32 x, f32 y, f32 z);
	Quaternion(f32 x, f32 y, f32 z, f32 w);

	const Quaternion operator- () const;
	
	f32 m_X;
	f32 m_Y;
	f32 m_Z;
	f32 m_W;
};

bool IsEqual(const Quaternion& quat1, const Quaternion& quat2, f32 epsilon);
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
