#include "Math/Vector2f.h"
#include "Math/Math.h"

const Vector2f Vector2f::ONE(1.0f, 1.0f);
const Vector2f Vector2f::ZERO(0.0f, 0.0f);
const Vector2f Vector2f::UP(0.0f, 1.0f);
const Vector2f Vector2f::DOWN(0.0f, -1.0f);
const Vector2f Vector2f::LEFT(-1.0f, 0.0f);
const Vector2f Vector2f::RIGHT(1.0f, 0.0f);

Vector2f::Vector2f()
	: m_X(0.0f)
	, m_Y(0.0f)
{
}

Vector2f::Vector2f(f32 x, f32 y)
	: m_X(x)
	, m_Y(y)
{
}

Vector2f::Vector2f(f32 scalar)
	: m_X(scalar)
	, m_Y(scalar)
{
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

f32 LengthSq(const Vector2f& vec)
{
	return Dot(vec, vec);
}

const Vector2f Normalize(const Vector2f& vec)
{
	return (Rcp(Length(vec)) * vec);
}

const Vector2f Negate(const Vector2f& vec)
{
	return (-1.0f * vec);
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

Vector2f& operator+= (Vector2f& left, const Vector2f& right)
{
	left.m_X += right.m_X;
	left.m_Y += right.m_Y;
	return left;
}

Vector2f& operator-= (Vector2f& left, const Vector2f& right)
{
	left.m_X -= right.m_X;
	left.m_Y -= right.m_Y;
	return left;
}

Vector2f& operator*= (Vector2f& left, const Vector2f& right)
{
	left.m_X *= right.m_X;
	left.m_Y *= right.m_Y;
	return left;
}

Vector2f& operator/= (Vector2f& left, const Vector2f& right)
{
	left.m_X /= right.m_X;
	left.m_Y /= right.m_Y;
	return left;
}

Vector2f& operator+= (Vector2f& left, f32 scalar)
{
	left.m_X += scalar;
	left.m_Y += scalar;
	return left;
}

Vector2f& operator-= (Vector2f& left, f32 scalar)
{
	left.m_X -= scalar;
	left.m_Y -= scalar;
	return left;
}

Vector2f& operator*= (Vector2f& left, f32 scalar)
{
	left.m_X *= scalar;
	left.m_Y *= scalar;
	return left;
}

Vector2f& operator/= (Vector2f& left, f32 scalar)
{
	left.m_X /= scalar;
	left.m_Y /= scalar;
	return left;
}

const Vector2f operator+ (const Vector2f& left, const Vector2f& right)
{
	return (Vector2f(left) += right);
}

const Vector2f operator- (const Vector2f& left, const Vector2f& right)
{
	return (Vector2f(left) -= right);
}

const Vector2f operator* (const Vector2f& left, const Vector2f& right)
{
	return (Vector2f(left) *= right);
}

const Vector2f operator/ (const Vector2f& left, const Vector2f& right)
{
	return (Vector2f(left) /= right);
}

const Vector2f operator+ (const Vector2f& left, f32 scalar)
{
	return (Vector2f(left) += scalar);
}

const Vector2f operator- (const Vector2f& left, f32 scalar)
{
	return (Vector2f(left) -= scalar);
}

const Vector2f operator* (const Vector2f& left, f32 scalar)
{
	return (Vector2f(left) *= scalar);
}

const Vector2f operator/ (const Vector2f& left, f32 scalar)
{
	return (Vector2f(left) /= scalar);
}
