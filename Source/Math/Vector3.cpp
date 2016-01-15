#include "Math/Vector3.h"

const Vector3f Vector3f::ONE(1.0f, 1.0f, 1.0f);
const Vector3f Vector3f::ZERO(0.0f, 0.0f, 0.0f);
const Vector3f Vector3f::UP(0.0f, 1.0f, 0.0f);
const Vector3f Vector3f::DOWN(0.0f, -1.0f, 0.0f);
const Vector3f Vector3f::LEFT(-1.0f, 0.0f, 0.0f);
const Vector3f Vector3f::RIGHT(1.0f, 0.0f, 0.0f);
const Vector3f Vector3f::BACK(0.0f, 0.0f, -1.0f);
const Vector3f Vector3f::FORWARD(0.0f, 0.0f, 1.0f);

Vector3f::Vector3f()
    : Vector3f(0.0f)
{
}

Vector3f::Vector3f(f32 x, f32 y, f32 z)
    : m_X(x)
    , m_Y(y)
    , m_Z(z)
{
}

Vector3f::Vector3f(f32 scalar)
    : Vector3f(scalar, scalar, scalar)
{
}

const Vector3f Vector3f::operator- () const
{
	return Vector3f(-m_X, -m_Y, -m_Z);
}

const Vector3f Abs(const Vector3f& vec)
{
    return Vector3f(Abs(vec.m_X), Abs(vec.m_Y), Abs(vec.m_Z));
}

const Vector3f Sqrt(const Vector3f& vec)
{
    return Vector3f(Sqrt(vec.m_X), Sqrt(vec.m_Y), Sqrt(vec.m_Z));
}

f32 Length(const Vector3f& vec)
{
    return Sqrt(Dot(vec, vec));
}

f32 LengthSquared(const Vector3f& vec)
{
    return Dot(vec, vec);
}

const Vector3f Normalize(const Vector3f& vec)
{
    return (Rcp(Length(vec)) * vec);
}

const Vector3f Rcp(const Vector3f& vec)
{
    return Vector3f(Rcp(vec.m_X), Rcp(vec.m_Y), Rcp(vec.m_Z));
}

f32 Dot(const Vector3f& vec1, const Vector3f& vec2)
{
    return (vec1.m_X * vec2.m_X + vec1.m_Y * vec2.m_Y + vec1.m_Z * vec2.m_Z);
}

const Vector3f Cross(const Vector3f& vec1, const Vector3f& vec2)
{
    return Vector3f(vec1.m_Y * vec2.m_Z - vec1.m_Z * vec2.m_Y,
        vec1.m_Z * vec2.m_X - vec1.m_X * vec2.m_Z,
        vec1.m_X * vec2.m_Y - vec1.m_Y * vec2.m_X);
}

bool IsEqual(const Vector3f& vec1, const Vector3f& vec2, f32 epsilon)
{
    return (::IsEqual(vec1.m_X, vec2.m_X, epsilon) && 
        ::IsEqual(vec1.m_Y, vec2.m_Y, epsilon) &&
        ::IsEqual(vec1.m_Z, vec2.m_Z, epsilon));
}

const Vector3f Min(const Vector3f& vec1, const Vector3f& vec2)
{
    return Vector3f(Min(vec1.m_X, vec2.m_X), Min(vec1.m_Y, vec2.m_Y), Min(vec1.m_Z, vec2.m_Z));
}

const Vector3f Max(const Vector3f& vec1, const Vector3f& vec2)
{
    return Vector3f(Max(vec1.m_X, vec2.m_X), Max(vec1.m_Y, vec2.m_Y), Max(vec1.m_Z, vec2.m_Z));
}

bool IsNormalized(const Vector3f& vec, f32 epsilon)
{
	return (Abs(1.0f - Length(vec)) < epsilon);
}

Vector3f& operator+= (Vector3f& vec1, const Vector3f& vec2)
{
	vec1.m_X += vec2.m_X;
	vec1.m_Y += vec2.m_Y;
	vec1.m_Z += vec2.m_Z;
    return vec1;
}

Vector3f& operator-= (Vector3f& vec1, const Vector3f& vec2)
{
	vec1.m_X -= vec2.m_X;
	vec1.m_Y -= vec2.m_Y;
	vec1.m_Z -= vec2.m_Z;
    return vec1;
}

Vector3f& operator*= (Vector3f& vec1, const Vector3f& vec2)
{
	vec1.m_X *= vec2.m_X;
	vec1.m_Y *= vec2.m_Y;
	vec1.m_Z *= vec2.m_Z;
    return vec1;
}

Vector3f& operator/= (Vector3f& vec1, const Vector3f& vec2)
{
	vec1.m_X /= vec2.m_X;
	vec1.m_Y /= vec2.m_Y;
	vec1.m_Z /= vec2.m_Z;
    return vec1;
}

Vector3f& operator+= (Vector3f& vec, f32 scalar)
{
	vec.m_X += scalar;
	vec.m_Y += scalar;
	vec.m_Z += scalar;
    return vec;
}

Vector3f& operator-= (Vector3f& vec, f32 scalar)
{
	vec.m_X -= scalar;
	vec.m_Y -= scalar;
	vec.m_Z -= scalar;
    return vec;
}

Vector3f& operator*= (Vector3f& vec, f32 scalar)
{
	vec.m_X *= scalar;
	vec.m_Y *= scalar;
	vec.m_Z *= scalar;
    return vec;
}

Vector3f& operator/= (Vector3f& vec, f32 scalar)
{
	vec.m_X /= scalar;
	vec.m_Y /= scalar;
	vec.m_Z /= scalar;
    return vec;
}

const Vector3f operator+ (const Vector3f& vec1, const Vector3f& vec2)
{
    return Vector3f(vec1.m_X + vec2.m_X, vec1.m_Y + vec2.m_Y, vec1.m_Z + vec2.m_Z);
}

const Vector3f operator- (const Vector3f& vec1, const Vector3f& vec2)
{
	return Vector3f(vec1.m_X - vec2.m_X, vec1.m_Y - vec2.m_Y, vec1.m_Z - vec2.m_Z);
}

const Vector3f operator* (const Vector3f& vec1, const Vector3f& vec2)
{
	return Vector3f(vec1.m_X * vec2.m_X, vec1.m_Y * vec2.m_Y, vec1.m_Z * vec2.m_Z);
}

const Vector3f operator/ (const Vector3f& vec1, const Vector3f& vec2)
{
	return Vector3f(vec1.m_X / vec2.m_X, vec1.m_Y / vec2.m_Y, vec1.m_Z / vec2.m_Z);
}

const Vector3f operator+ (const Vector3f& vec, f32 scalar)
{
    return Vector3f(vec.m_X + scalar, vec.m_Y + scalar, vec.m_Z + scalar);
}

const Vector3f operator+ (f32 scalar, const Vector3f& vec)
{
	return Vector3f(scalar + vec.m_X, scalar + vec.m_Y, scalar + vec.m_Z);
}

const Vector3f operator- (const Vector3f& vec, f32 scalar)
{
    return Vector3f(vec.m_X - scalar, vec.m_Y - scalar, vec.m_Z - scalar);
}

const Vector3f operator- (f32 scalar, const Vector3f& vec)
{
	return Vector3f(scalar - vec.m_X, scalar - vec.m_Y, scalar - vec.m_Z);
}

const Vector3f operator* (const Vector3f& vec, f32 scalar)
{
    return Vector3f(vec.m_X * scalar, vec.m_Y * scalar, vec.m_Z * scalar);
}

const Vector3f operator* (f32 scalar, const Vector3f& vec)
{
	return Vector3f(scalar * vec.m_X, scalar * vec.m_Y, scalar * vec.m_Z);
}

const Vector3f operator/ (const Vector3f& vec, f32 scalar)
{
    return Vector3f(vec.m_X / scalar, vec.m_Y / scalar, vec.m_Z / scalar);
}

const Vector3f operator/ (f32 scalar, const Vector3f& vec)
{
	return Vector3f(scalar / vec.m_X, scalar / vec.m_Y, scalar / vec.m_Z);
}

const Vector3i Vector3i::ONE(1, 1, 1);
const Vector3i Vector3i::ZERO(0, 0, 0);
const Vector3i Vector3i::UP(0, 1, 0);
const Vector3i Vector3i::DOWN(0, -1, 0);
const Vector3i Vector3i::LEFT(-1, 0, 0);
const Vector3i Vector3i::RIGHT(1, 0, 0);
const Vector3i Vector3i::BACK(0, 0, -1);
const Vector3i Vector3i::FORWARD(0, 0, 1);

Vector3i::Vector3i()
	: Vector3i(0)
{
}

Vector3i::Vector3i(i32 x, i32 y, i32 z)
	: m_X(x)
	, m_Y(y)
	, m_Z(z)
{
}

Vector3i::Vector3i(i32 scalar)
	: Vector3i(scalar, scalar, scalar)
{
}

const Vector3i Vector3i::operator- () const
{
	return Vector3i(-m_X, -m_Y, -m_Z);
}

const Vector3u Vector3u::ONE(1, 1, 1);
const Vector3u Vector3u::ZERO(0, 0, 0);

Vector3u::Vector3u()
	: Vector3u(0)
{
}

Vector3u::Vector3u(u32 x, u32 y, u32 z)
	: m_X(x)
	, m_Y(y)
	, m_Z(z)
{
}

Vector3u::Vector3u(u32 scalar)
	: Vector3u(scalar, scalar, scalar)
{
}