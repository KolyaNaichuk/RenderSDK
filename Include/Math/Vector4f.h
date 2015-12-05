#pragma once

#include "Common/Common.h"

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
f32 LengthSq(const Vector4f& vec);
const Vector4f Normalize(const Vector4f& vec);
const Vector4f Negate(const Vector4f& vec);
const Vector4f Rcp(const Vector4f& vec);
f32 Dot(const Vector4f& vec1, const Vector4f& vec2);
bool  IsEqual(const Vector4f& vec1, const Vector4f& vec2, f32 epsilon);
const Vector4f Min(const Vector4f& vec1, const Vector4f& vec2);
const Vector4f Max(const Vector4f& vec1, const Vector4f& vec2);

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
