#pragma once

#include "Math/Math.h"

struct Vector4f
{
    Vector4f();
    Vector4f(f32 x, f32 y, f32 z, f32 w);
    Vector4f(f32 scalar);
        
    f32 m_X;
    f32 m_Y;
    f32 m_Z;
    f32 m_W;

	static const Vector4f ONE;
	static const Vector4f ZERO;
};

const Vector4f Abs(const Vector4f& vec);
const Vector4f Sqrt(const Vector4f& vec);
f32 Length(const Vector4f& vec);
f32 LengthSquared(const Vector4f& vec);
const Vector4f Normalize(const Vector4f& vec);
const Vector4f Negate(const Vector4f& vec);
const Vector4f Rcp(const Vector4f& vec);
f32 Dot(const Vector4f& vec1, const Vector4f& vec2);
bool IsEqual(const Vector4f& vec1, const Vector4f& vec2, f32 epsilon = EPSILON);
const Vector4f Min(const Vector4f& vec1, const Vector4f& vec2);
const Vector4f Max(const Vector4f& vec1, const Vector4f& vec2);
bool IsNormalized(const Vector4f& vec, f32 epsilon = EPSILON);

Vector4f& operator+= (Vector4f& left, const Vector4f& right);
Vector4f& operator-= (Vector4f& left, const Vector4f& right);
Vector4f& operator*= (Vector4f& left, const Vector4f& right);
Vector4f& operator/= (Vector4f& left, const Vector4f& right);

Vector4f& operator+= (Vector4f& left, f32 scalar);
Vector4f& operator-= (Vector4f& left, f32 scalar);
Vector4f& operator*= (Vector4f& left, f32 scalar);
Vector4f& operator/= (Vector4f& left, f32 scalar);

const Vector4f operator+ (const Vector4f& left, const Vector4f& right);
const Vector4f operator- (const Vector4f& left, const Vector4f& right);
const Vector4f operator* (const Vector4f& left, const Vector4f& right);
const Vector4f operator/ (const Vector4f& left, const Vector4f& right);

const Vector4f operator+ (const Vector4f& left, f32 scalar);
const Vector4f operator- (const Vector4f& left, f32 scalar);
const Vector4f operator* (const Vector4f& left, f32 scalar);
const Vector4f operator/ (const Vector4f& left, f32 scalar);

struct Vector4i
{
	Vector4i();
	Vector4i(i32 x, i32 y, i32 z, i32 w);
	Vector4i(i32 scalar);

	i32 m_X;
	i32 m_Y;
	i32 m_Z;
	i32 m_W;

	static const Vector4i ONE;
	static const Vector4i ZERO;
};

struct Vector4u
{
	Vector4u();
	Vector4u(u32 x, u32 y, u32 z, u32 w);
	Vector4u(u32 scalar);

	u32 m_X;
	u32 m_Y;
	u32 m_Z;
	u32 m_W;

	static const Vector4u ONE;
	static const Vector4u ZERO;
};