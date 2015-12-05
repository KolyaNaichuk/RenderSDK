#pragma once

#include "Common/Common.h"

struct Vector2f
{
	Vector2f();
	Vector2f(f32 x, f32 y);
	Vector2f(f32 scalar);

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
f32 LengthSq(const Vector2f& vec);
const Vector2f Normalize(const Vector2f& vec);
const Vector2f Negate(const Vector2f& vec);
const Vector2f Rcp(const Vector2f& vec);
f32 Dot(const Vector2f& vec1, const Vector2f& vec2);
bool IsEqual(const Vector2f& vec1, const Vector2f& vec2, f32 epsilon);
const Vector2f Min(const Vector2f& vec1, const Vector2f& vec2);
const Vector2f Max(const Vector2f& vec1, const Vector2f& vec2);

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