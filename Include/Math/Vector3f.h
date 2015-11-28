#pragma once

#include "Common/Common.h"

struct Vector3f
{
    Vector3f();
    Vector3f(f32 x, f32 y, f32 z);
    Vector3f(f32 scalar);

    f32 m_X;
    f32 m_Y;
    f32 m_Z;
};

const Vector3f Abs(const Vector3f& vec);
const Vector3f Sqrt(const Vector3f& vec);
f32 Length(const Vector3f& vec);
f32 LengthSq(const Vector3f& vec);
const Vector3f Normalize(const Vector3f& vec);
const Vector3f Negate(const Vector3f& vec);
const Vector3f Rcp(const Vector3f& vec);
f32 Dot(const Vector3f& vec1, const Vector3f& vec2);
const Vector3f Cross(const Vector3f& vec1, const Vector3f& vec2);
bool  IsEqual(const Vector3f& vec1, const Vector3f& vec2, f32 epsilon);
const Vector3f Min(const Vector3f& vec1, const Vector3f& vec2);
const Vector3f Max(const Vector3f& vec1, const Vector3f& vec2);

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
