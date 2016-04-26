#pragma once

#include "Math/Math.h"

class Transform;

struct Vector4f
{
	explicit Vector4f();
	explicit Vector4f(f32 x, f32 y, f32 z, f32 w);
    explicit Vector4f(f32 scalar);
    
	f32& operator[] (u8 index);
	const f32& operator[] (u8 index) const;

	const Vector4f operator- () const;

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
const Vector4f Rcp(const Vector4f& vec);
f32 Dot(const Vector4f& vec1, const Vector4f& vec2);
bool IsEqual(const Vector4f& vec1, const Vector4f& vec2, f32 epsilon = EPSILON);
const Vector4f Min(const Vector4f& vec1, const Vector4f& vec2);
const Vector4f Max(const Vector4f& vec1, const Vector4f& vec2);
bool IsNormalized(const Vector4f& vec, f32 epsilon = EPSILON);
const Vector4f TransformVector(const Vector4f& vec, const Transform& transform);
const Vector4f TransformNormal(const Vector4f& vec, const Transform& transform);

Vector4f& operator+= (Vector4f& vec1, const Vector4f& vec2);
Vector4f& operator-= (Vector4f& vec1, const Vector4f& vec2);
Vector4f& operator*= (Vector4f& vec1, const Vector4f& vec2);
Vector4f& operator/= (Vector4f& vec1, const Vector4f& vec2);

Vector4f& operator+= (Vector4f& vec, f32 scalar);
Vector4f& operator-= (Vector4f& vec, f32 scalar);
Vector4f& operator*= (Vector4f& vec, f32 scalar);
Vector4f& operator/= (Vector4f& vec, f32 scalar);

const Vector4f operator+ (const Vector4f& vec1, const Vector4f& vec2);
const Vector4f operator- (const Vector4f& vec1, const Vector4f& vec2);
const Vector4f operator* (const Vector4f& vec1, const Vector4f& vec2);
const Vector4f operator/ (const Vector4f& vec1, const Vector4f& vec2);

const Vector4f operator+ (const Vector4f& vec, f32 scalar);
const Vector4f operator+ (f32 scalar, const Vector4f& vec);

const Vector4f operator- (const Vector4f& vec, f32 scalar);
const Vector4f operator- (f32 scalar, const Vector4f& vec);

const Vector4f operator* (const Vector4f& vec, f32 scalar);
const Vector4f operator* (f32 scalar, const Vector4f& vec);

const Vector4f operator/ (const Vector4f& vec, f32 scalar);
const Vector4f operator/ (f32 scalar, const Vector4f& vec);

struct Vector4i
{
	explicit Vector4i();
	explicit Vector4i(i32 x, i32 y, i32 z, i32 w);
	explicit Vector4i(i32 scalar);
	
	i32& operator[] (u8 index);
	const i32& operator[] (u8 index) const;

	const Vector4i operator- () const;

	i32 m_X;
	i32 m_Y;
	i32 m_Z;
	i32 m_W;

	static const Vector4i ONE;
	static const Vector4i ZERO;
};

struct Vector4u
{
	explicit Vector4u();
	explicit Vector4u(u32 x, u32 y, u32 z, u32 w);
	explicit Vector4u(u32 scalar);

	u32& operator[] (u8 index);
	const u32& operator[] (u8 index) const;

	u32 m_X;
	u32 m_Y;
	u32 m_Z;
	u32 m_W;

	static const Vector4u ONE;
	static const Vector4u ZERO;
};