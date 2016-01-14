#pragma once

#include "Math/Math.h"

struct Vector2f
{
	Vector2f();
	Vector2f(f32 x, f32 y);
	explicit Vector2f(f32 scalar);

	f32 m_X;
	f32 m_Y;

	static const Vector2f ONE;
	static const Vector2f ZERO;
	static const Vector2f UP;
	static const Vector2f DOWN;
	static const Vector2f LEFT;
	static const Vector2f RIGHT;
};

const Vector2f Abs(const Vector2f& vec);
const Vector2f Sqrt(const Vector2f& vec);
f32 Length(const Vector2f& vec);
f32 LengthSquared(const Vector2f& vec);
const Vector2f Normalize(const Vector2f& vec);
const Vector2f Negate(const Vector2f& vec);
const Vector2f Rcp(const Vector2f& vec);
f32 Dot(const Vector2f& vec1, const Vector2f& vec2);
bool IsEqual(const Vector2f& vec1, const Vector2f& vec2, f32 epsilon = EPSILON);
const Vector2f Min(const Vector2f& vec1, const Vector2f& vec2);
const Vector2f Max(const Vector2f& vec1, const Vector2f& vec2);
bool IsNormalized(const Vector2f& vec, f32 epsilon = EPSILON);

Vector2f& operator+= (Vector2f& left, const Vector2f& right);
Vector2f& operator-= (Vector2f& left, const Vector2f& right);
Vector2f& operator*= (Vector2f& left, const Vector2f& right);
Vector2f& operator/= (Vector2f& left, const Vector2f& right);

Vector2f& operator+= (Vector2f& left, f32 scalar);
Vector2f& operator-= (Vector2f& left, f32 scalar);
Vector2f& operator*= (Vector2f& left, f32 scalar);
Vector2f& operator/= (Vector2f& left, f32 scalar);

const Vector2f operator+ (const Vector2f& left, const Vector2f& right);
const Vector2f operator- (const Vector2f& left, const Vector2f& right);
const Vector2f operator* (const Vector2f& left, const Vector2f& right);
const Vector2f operator/ (const Vector2f& left, const Vector2f& right);

const Vector2f operator+ (const Vector2f& left, f32 scalar);
const Vector2f operator- (const Vector2f& left, f32 scalar);
const Vector2f operator* (const Vector2f& left, f32 scalar);
const Vector2f operator/ (const Vector2f& left, f32 scalar);

struct Vector2i
{
	Vector2i();
	Vector2i(i32 x, i32 y);
	Vector2i(i32 scalar);

	i32 m_X;
	i32 m_Y;

	static const Vector2i ONE;
	static const Vector2i ZERO;
};

struct Vector2u
{
	Vector2u();
	Vector2u(u32 x, u32 y);
	Vector2u(u32 scalar);

	u32 m_X;
	u32 m_Y;

	static const Vector2u ONE;
	static const Vector2u ZERO;
};