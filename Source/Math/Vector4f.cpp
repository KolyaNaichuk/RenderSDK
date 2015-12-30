#include "Math/Vector4f.h"
#include "Math/Math.h"

const Vector4f Vector4f::ONE(1.0f, 1.0f, 1.0f, 1.0f);
const Vector4f Vector4f::ZERO(0.0f, 0.0f, 0.0f, 0.0f);

Vector4f::Vector4f()
    : m_X(0.0f)
    , m_Y(0.0f)
    , m_Z(0.0f)
    , m_W(0.0f)
{
}

Vector4f::Vector4f(f32 x, f32 y, f32 z, f32 w)
    : m_X(x)
    , m_Y(y)
    , m_Z(z)
    , m_W(w)
{
}

Vector4f::Vector4f(f32 scalar)
    : m_X(scalar)
    , m_Y(scalar)
    , m_Z(scalar)
    , m_W(scalar)
{
}

const Vector4f Abs(const Vector4f& vec)
{
    return Vector4f(Abs(vec.m_X), Abs(vec.m_Y), Abs(vec.m_Z), Abs(vec.m_W));
}

const Vector4f Sqrt(const Vector4f& vec)
{
    return Vector4f(Sqrt(vec.m_X), Sqrt(vec.m_Y), Sqrt(vec.m_Z), Sqrt(vec.m_W));
}

f32 Length(const Vector4f& vec)
{
    return Sqrt(Dot(vec, vec));
}

f32 LengthSquared(const Vector4f& vec)
{
    return Dot(vec, vec);
}

const Vector4f Normalize(const Vector4f& vec)
{
    return (Rcp(Length(vec)) * vec);
}

const Vector4f Negate(const Vector4f& vec)
{
    return (-1.0f * vec);
}

const Vector4f Rcp(const Vector4f& vec)
{
    return Vector4f(Rcp(vec.m_X), Rcp(vec.m_Y), Rcp(vec.m_Z), Rcp(vec.m_W));
}

f32 Dot(const Vector4f& vec1, const Vector4f& vec2)
{
    return (vec1.m_X * vec2.m_X + vec1.m_Y * vec2.m_Y + vec1.m_Z * vec2.m_Z + vec1.m_W * vec2.m_W);
}

bool IsEqual(const Vector4f& vec1, const Vector4f& vec2, f32 epsilon)
{
    return (::IsEqual(vec1.m_X, vec2.m_X, epsilon) &&
        ::IsEqual(vec1.m_Y, vec2.m_Y, epsilon) &&
        ::IsEqual(vec1.m_Z, vec2.m_Z, epsilon) &&
        ::IsEqual(vec1.m_W, vec2.m_W, epsilon));
}

const Vector4f Min(const Vector4f& vec1, const Vector4f& vec2)
{
    return Vector4f(Min(vec1.m_X, vec2.m_X), Min(vec1.m_Y, vec2.m_Y), Min(vec1.m_Z, vec2.m_Z), Min(vec1.m_W, vec2.m_W));
}

const Vector4f Max(const Vector4f& vec1, const Vector4f& vec2)
{
    return Vector4f(Max(vec1.m_X, vec2.m_X), Max(vec1.m_Y, vec2.m_Y), Max(vec1.m_Z, vec2.m_Z), Max(vec1.m_W, vec2.m_W));
}

bool IsNormalized(const Vector4f& vec, f32 epsilon)
{
	return (Abs(1.0f - Length(vec)) < epsilon);
}

Vector4f& operator+= (Vector4f& left, const Vector4f& right)
{
    left.m_X += right.m_X;
    left.m_Y += right.m_Y;
    left.m_Z += right.m_Z;
    left.m_W += right.m_W;
    return left;
}

Vector4f& operator-= (Vector4f& left, const Vector4f& right)
{
    left.m_X -= right.m_X;
    left.m_Y -= right.m_Y;
    left.m_Z -= right.m_Z;
    left.m_W -= right.m_W;
    return left;
}

Vector4f& operator*= (Vector4f& left, const Vector4f& right)
{
    left.m_X *= right.m_X;
    left.m_Y *= right.m_Y;
    left.m_Z *= right.m_Z;
    left.m_W *= right.m_W;
    return left;
}

Vector4f& operator/= (Vector4f& left, const Vector4f& right)
{
    left.m_X /= right.m_X;
    left.m_Y /= right.m_Y;
    left.m_Z /= right.m_Z;
    left.m_W /= right.m_W;
    return left;
}

Vector4f& operator+= (Vector4f& left, f32 scalar)
{
	left.m_X += scalar;
	left.m_Y += scalar;
	left.m_Z += scalar;
	left.m_W += scalar;
    return left;
}

Vector4f& operator-= (Vector4f& left, f32 scalar)
{
	left.m_X -= scalar;
	left.m_Y -= scalar;
	left.m_Z -= scalar;
	left.m_W -= scalar;
    return left;
}

Vector4f& operator*= (Vector4f& left, f32 scalar)
{
	left.m_X *= scalar;
	left.m_Y *= scalar;
	left.m_Z *= scalar;
	left.m_W *= scalar;
    return left;
}

Vector4f& operator/= (Vector4f& left, f32 scalar)
{
	left.m_X /= scalar;
	left.m_Y /= scalar;
	left.m_Z /= scalar;
	left.m_W /= scalar;
    return left;
}

const Vector4f operator+ (const Vector4f& left, const Vector4f& right)
{
    return (Vector4f(left) += right);
}

const Vector4f operator- (const Vector4f& left, const Vector4f& right)
{
    return (Vector4f(left) -= right);
}

const Vector4f operator* (const Vector4f& left, const Vector4f& right)
{
    return (Vector4f(left) *= right);
}

const Vector4f operator/ (const Vector4f& left, const Vector4f& right)
{
    return (Vector4f(left) /= right);
}

const Vector4f operator+ (const Vector4f& left, f32 scalar)
{
    return (Vector4f(left) += scalar);
}

const Vector4f operator- (const Vector4f& left, f32 scalar)
{
    return (Vector4f(left) -= scalar);
}

const Vector4f operator* (const Vector4f& left, f32 scalar)
{
    return (Vector4f(left) *= scalar);
}

const Vector4f operator/ (const Vector4f& left, f32 scalar)
{
    return (Vector4f(left) /= scalar);
}
