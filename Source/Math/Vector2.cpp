#include "Math/Vector2.h"

const Vector2f Vector2f::ONE(1.0f, 1.0f);
const Vector2f Vector2f::ZERO(0.0f, 0.0f);
const Vector2f Vector2f::UP(0.0f, 1.0f);
const Vector2f Vector2f::DOWN(0.0f, -1.0f);
const Vector2f Vector2f::LEFT(-1.0f, 0.0f);
const Vector2f Vector2f::RIGHT(1.0f, 0.0f);

Vector2f::Vector2f()
	: Vector2f(0.0f)
{
}

Vector2f::Vector2f(f32 x, f32 y)
	: m_X(x)
	, m_Y(y)
{
}

Vector2f::Vector2f(f32 scalar)
	: Vector2f(scalar, scalar)
{
}

const Vector2f Vector2f::operator- () const
{
	return Vector2f(-m_X, -m_Y);
}

const Vector2f Abs(const Vector2f& vec)
{
	return Vector2f(Abs(vec.m_X), Abs(vec.m_Y));
}

const Vector2f Sqrt(const Vector2f& vec)
{
	return Vector2f(Sqrt(vec.m_X), Sqrt(vec.m_Y));
}

f32 Length(const Vector2f& vec)
{
	return Sqrt(Dot(vec, vec));
}

f32 LengthSquared(const Vector2f& vec)
{
	return Dot(vec, vec);
}

const Vector2f Normalize(const Vector2f& vec)
{
	return (Rcp(Length(vec)) * vec);
}

const Vector2f Rcp(const Vector2f& vec)
{
	return Vector2f(Rcp(vec.m_X), Rcp(vec.m_Y));
}

f32 Dot(const Vector2f& vec1, const Vector2f& vec2)
{
	return (vec1.m_X * vec2.m_X + vec1.m_Y * vec2.m_Y);
}

bool IsEqual(const Vector2f& vec1, const Vector2f& vec2, f32 epsilon)
{
	return (::IsEqual(vec1.m_X, vec2.m_X, epsilon) && ::IsEqual(vec1.m_Y, vec2.m_Y, epsilon));
}

const Vector2f Min(const Vector2f& vec1, const Vector2f& vec2)
{
	return Vector2f(Min(vec1.m_X, vec2.m_X), Min(vec1.m_Y, vec2.m_Y));
}

const Vector2f Max(const Vector2f& vec1, const Vector2f& vec2)
{
	return Vector2f(Max(vec1.m_X, vec2.m_X), Max(vec1.m_Y, vec2.m_Y));
}

bool IsNormalized(const Vector2f& vec, f32 epsilon)
{
	return (Abs(1.0f - Length(vec)) < epsilon);
}

Vector2f& operator+= (Vector2f& vec1, const Vector2f& vec2)
{
	vec1.m_X += vec2.m_X;
	vec1.m_Y += vec2.m_Y;
	return vec1;
}

Vector2f& operator-= (Vector2f& vec1, const Vector2f& vec2)
{
	vec1.m_X -= vec2.m_X;
	vec1.m_Y -= vec2.m_Y;
	return vec1;
}

Vector2f& operator*= (Vector2f& vec1, const Vector2f& vec2)
{
	vec1.m_X *= vec2.m_X;
	vec1.m_Y *= vec2.m_Y;
	return vec1;
}

Vector2f& operator/= (Vector2f& vec1, const Vector2f& vec2)
{
	vec1.m_X /= vec2.m_X;
	vec1.m_Y /= vec2.m_Y;
	return vec1;
}

Vector2f& operator+= (Vector2f& vec, f32 scalar)
{
	vec.m_X += scalar;
	vec.m_Y += scalar;
	return vec;
}

Vector2f& operator-= (Vector2f& vec, f32 scalar)
{
	vec.m_X -= scalar;
	vec.m_Y -= scalar;
	return vec;
}

Vector2f& operator*= (Vector2f& vec, f32 scalar)
{
	vec.m_X *= scalar;
	vec.m_Y *= scalar;
	return vec;
}

Vector2f& operator/= (Vector2f& vec, f32 scalar)
{
	f32 rcpScalar = Rcp(scalar);
	vec.m_X *= rcpScalar;
	vec.m_Y *= rcpScalar;
	return vec;
}

const Vector2f operator+ (const Vector2f& vec1, const Vector2f& vec2)
{
	return Vector2f(vec1.m_X + vec2.m_X, vec1.m_Y + vec2.m_Y);
}

const Vector2f operator- (const Vector2f& vec1, const Vector2f& vec2)
{
	return Vector2f(vec1.m_X - vec2.m_X, vec1.m_Y - vec2.m_Y);
}

const Vector2f operator* (const Vector2f& vec1, const Vector2f& vec2)
{
	return Vector2f(vec1.m_X * vec2.m_X, vec1.m_Y * vec2.m_Y);
}

const Vector2f operator/ (const Vector2f& vec1, const Vector2f& vec2)
{
	return Vector2f(vec1.m_X / vec2.m_X, vec1.m_Y / vec2.m_Y);
}

const Vector2f operator+ (const Vector2f& vec, f32 scalar)
{
	return Vector2f(vec.m_X + scalar, vec.m_Y + scalar);
}

const Vector2f operator+ (f32 scalar, const Vector2f& vec)
{
	return Vector2f(scalar + vec.m_X, scalar + vec.m_Y);
}

const Vector2f operator- (const Vector2f& vec, f32 scalar)
{
	return Vector2f(vec.m_X - scalar, vec.m_Y - scalar);
}

const Vector2f operator- (f32 scalar, const Vector2f& vec)
{
	return Vector2f(scalar - vec.m_X, scalar - vec.m_Y);
}

const Vector2f operator* (const Vector2f& vec, f32 scalar)
{
	return Vector2f(vec.m_X * scalar, vec.m_Y * scalar);
}

const Vector2f operator* (f32 scalar, const Vector2f& vec)
{
	return Vector2f(scalar * vec.m_X, scalar * vec.m_Y);
}

const Vector2f operator/ (const Vector2f& vec, f32 scalar)
{
	f32 rcpScalar = Rcp(scalar);
	return Vector2f(vec.m_X * rcpScalar, vec.m_Y * rcpScalar);
}

const Vector2f operator/ (f32 scalar, const Vector2f& vec)
{
	return Vector2f(scalar / vec.m_X, scalar / vec.m_Y);
}

const Vector2i Vector2i::ONE(1, 1);
const Vector2i Vector2i::ZERO(0, 0);

Vector2i::Vector2i()
	: Vector2i(0)
{
}

Vector2i::Vector2i(i32 x, i32 y)
	: m_X(x)
	, m_Y(y)
{
}

Vector2i::Vector2i(i32 scalar)
	: Vector2i(scalar, scalar)
{
}

const Vector2i Vector2i::operator- () const
{
	return Vector2i(-m_X, -m_Y);
}

const Vector2u Vector2u::ONE(1, 1);
const Vector2u Vector2u::ZERO(0, 0);

Vector2u::Vector2u()
	: Vector2u(0)
{
}

Vector2u::Vector2u(u32 x, u32 y)
	: m_X(x)
	, m_Y(y)
{
}

Vector2u::Vector2u(u32 scalar)
	: Vector2u(scalar, scalar)
{
}
