#include "Math/Vector3f.h"
#include "Math/Math.h"

Vector3f::Vector3f()
    : m_X(0.0f)
    , m_Y(0.0f)
    , m_Z(0.0f)
{
}

Vector3f::Vector3f(f32 x, f32 y, f32 z)
    : m_X(x)
    , m_Y(y)
    , m_Z(z)
{
}

Vector3f::Vector3f(f32 scalar)
    : m_X(scalar)
    , m_Y(scalar)
    , m_Z(scalar)
{
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

f32 LengthSq(const Vector3f& vec)
{
    return Dot(vec, vec);
}

const Vector3f Normalize(const Vector3f& vec)
{
    return (Rcp(Length(vec)) * vec);
}

const Vector3f Negate(const Vector3f& vec)
{
    return (-1.0f * vec);
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

Vector3f& operator+= (Vector3f& left, const Vector3f& right)
{
    left.m_X += right.m_X;
    left.m_Y += right.m_Y;
    left.m_Z += right.m_Z;
    return left;
}

Vector3f& operator-= (Vector3f& left, const Vector3f& right)
{
    left.m_X -= right.m_X;
    left.m_Y -= right.m_Y;
    left.m_Z -= right.m_Z;
    return left;
}

Vector3f& operator*= (Vector3f& left, const Vector3f& right)
{
    left.m_X *= right.m_X;
    left.m_Y *= right.m_Y;
    left.m_Z *= right.m_Z;
    return left;
}

Vector3f& operator/= (Vector3f& left, const Vector3f& right)
{
    left.m_X /= right.m_X;
    left.m_Y /= right.m_Y;
    left.m_Z /= right.m_Z;
    return left;
}

Vector3f& operator+= (Vector3f& left, f32 scalar)
{
	left.m_X += scalar;
	left.m_Y += scalar;
	left.m_Z += scalar;
    return left;
}

Vector3f& operator-= (Vector3f& left, f32 scalar)
{
	left.m_X -= scalar;
	left.m_Y -= scalar;
	left.m_Z -= scalar;
    return left;
}

Vector3f& operator*= (Vector3f& left, f32 scalar)
{
	left.m_X *= scalar;
	left.m_Y *= scalar;
	left.m_Z *= scalar;
    return left;
}

Vector3f& operator/= (Vector3f& left, f32 scalar)
{
	left.m_X /= scalar;
	left.m_Y /= scalar;
	left.m_Z /= scalar;
    return left;
}

const Vector3f operator+ (const Vector3f& left, const Vector3f& right)
{
    return (Vector3f(left) += right);
}

const Vector3f operator- (const Vector3f& left, const Vector3f& right)
{
    return (Vector3f(left) -= right);
}

const Vector3f operator* (const Vector3f& left, const Vector3f& right)
{
    return (Vector3f(left) *= right);
}

const Vector3f operator/ (const Vector3f& left, const Vector3f& right)
{
    return (Vector3f(left) /= right);
}

const Vector3f operator+ (const Vector3f& left, f32 scalar)
{
    return (Vector3f(left) += scalar);
}

const Vector3f operator- (const Vector3f& left, f32 scalar)
{
    return (Vector3f(left) -= scalar);
}

const Vector3f operator* (const Vector3f& left, f32 scalar)
{
    return (Vector3f(left) *= scalar);
}

const Vector3f operator/ (const Vector3f& left, f32 scalar)
{
    return (Vector3f(left) /= scalar);
}