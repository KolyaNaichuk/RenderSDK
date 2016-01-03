#pragma once

#include "Math/Math.h"

struct Vector3f
{
    Vector3f();
    Vector3f(f32 x, f32 y, f32 z);
    Vector3f(f32 scalar);

    f32 m_X;
    f32 m_Y;
    f32 m_Z;

	static const Vector3f ONE;
	static const Vector3f ZERO;
	static const Vector3f UP;
	static const Vector3f DOWN;
	static const Vector3f LEFT;
	static const Vector3f RIGHT;
	static const Vector3f BACK;
	static const Vector3f FORWARD;
};

const Vector3f Abs(const Vector3f& vec);
const Vector3f Sqrt(const Vector3f& vec);
f32 Length(const Vector3f& vec);
f32 LengthSquared(const Vector3f& vec);
const Vector3f Normalize(const Vector3f& vec);
const Vector3f Negate(const Vector3f& vec);
const Vector3f Rcp(const Vector3f& vec);
f32 Dot(const Vector3f& vec1, const Vector3f& vec2);
const Vector3f Cross(const Vector3f& vec1, const Vector3f& vec2);
bool IsEqual(const Vector3f& vec1, const Vector3f& vec2, f32 epsilon = EPSILON);
const Vector3f Min(const Vector3f& vec1, const Vector3f& vec2);
const Vector3f Max(const Vector3f& vec1, const Vector3f& vec2);
bool IsNormalized(const Vector3f& vec, f32 epsilon = EPSILON);

Vector3f& operator+= (Vector3f& left, const Vector3f& right);
Vector3f& operator-= (Vector3f& left, const Vector3f& right);
Vector3f& operator*= (Vector3f& left, const Vector3f& right);
Vector3f& operator/= (Vector3f& left, const Vector3f& right);

Vector3f& operator+= (Vector3f& left, f32 scalar);
Vector3f& operator-= (Vector3f& left, f32 scalar);
Vector3f& operator*= (Vector3f& left, f32 scalar);
Vector3f& operator/= (Vector3f& left, f32 scalar);

const Vector3f operator+ (const Vector3f& left, const Vector3f& right);
const Vector3f operator- (const Vector3f& left, const Vector3f& right);
const Vector3f operator* (const Vector3f& left, const Vector3f& right);
const Vector3f operator/ (const Vector3f& left, const Vector3f& right);

const Vector3f operator+ (const Vector3f& left, f32 scalar);
const Vector3f operator- (const Vector3f& left, f32 scalar);
const Vector3f operator* (const Vector3f& left, f32 scalar);
const Vector3f operator/ (const Vector3f& left, f32 scalar);

struct Vector3i
{
	Vector3i();
	Vector3i(i32 x, i32 y, i32 z);
	Vector3i(i32 scalar);

	i32 m_X;
	i32 m_Y;
	i32 m_Z;

	static const Vector3i ONE;
	static const Vector3i ZERO;
	static const Vector3i UP;
	static const Vector3i DOWN;
	static const Vector3i LEFT;
	static const Vector3i RIGHT;
	static const Vector3i BACK;
	static const Vector3i FORWARD;
};

struct Vector3u
{
	Vector3u();
	Vector3u(u32 x, u32 y, u32 z);
	Vector3u(u32 scalar);

	u32 m_X;
	u32 m_Y;
	u32 m_Z;

	static const Vector3u ONE;
	static const Vector3u ZERO;
};