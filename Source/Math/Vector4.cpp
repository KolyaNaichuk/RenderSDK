#include "Math/Vector4.h"
#include "Math/Transform.h"

const Vector4f Vector4f::ONE(1.0f, 1.0f, 1.0f, 1.0f);
const Vector4f Vector4f::ZERO(0.0f, 0.0f, 0.0f, 0.0f);

Vector4f::Vector4f()
    : Vector4f(0.0f)
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
    : Vector4f(scalar, scalar, scalar, scalar)
{
}

f32& Vector4f::operator[] (u8 index)
{
	assert(index < 4);
	return *(&m_X + index);
}

const f32& Vector4f::operator[] (u8 index) const
{
	assert(index < 4);
	return *(&m_X + index);
}

const Vector4f Vector4f::operator- () const
{
	return Vector4f(-m_X, -m_Y, -m_Z, -m_W);
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

const Vector4f Rcp(const Vector4f& vec)
{
    return Vector4f(Rcp(vec.m_X), Rcp(vec.m_Y), Rcp(vec.m_Z), Rcp(vec.m_W));
}

f32 Dot(const Vector4f& vec1, const Vector4f& vec2)
{
    return (vec1.m_X * vec2.m_X + vec1.m_Y * vec2.m_Y + vec1.m_Z * vec2.m_Z + vec1.m_W * vec2.m_W);
}

bool AreEqual(const Vector4f& vec1, const Vector4f& vec2, f32 epsilon)
{
    return (::AreEqual(vec1.m_X, vec2.m_X, epsilon) &&
        ::AreEqual(vec1.m_Y, vec2.m_Y, epsilon) &&
        ::AreEqual(vec1.m_Z, vec2.m_Z, epsilon) &&
        ::AreEqual(vec1.m_W, vec2.m_W, epsilon));
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
	return AreEqual(1.0f, Length(vec), epsilon);
}

const Vector4f TransformVector(const Vector4f& vec, const Transform& transform)
{
	return vec * transform.GetLocalToWorldMatrix();
}

const Vector4f TransformNormal(const Vector4f& vec, const Transform& transform)
{
	return transform.GetWorldToLocalMatrix() * vec;
}

Vector4f ToHomogeneousVector(const Vector3f& cartesianVec)
{
	return Vector4f(cartesianVec.m_X, cartesianVec.m_Y, cartesianVec.m_Z, 0.0f);
}

Vector4f ToHomogeneousPoint(const Vector3f& cartesianPoint, f32 w)
{
	return Vector4f(cartesianPoint.m_X * w, cartesianPoint.m_Y * w, cartesianPoint.m_Z * w, w);
}

Vector4f& operator+= (Vector4f& vec1, const Vector4f& vec2)
{
	vec1.m_X += vec2.m_X;
	vec1.m_Y += vec2.m_Y;
	vec1.m_Z += vec2.m_Z;
	vec1.m_W += vec2.m_W;
    return vec1;
}

Vector4f& operator-= (Vector4f& vec1, const Vector4f& vec2)
{
	vec1.m_X -= vec2.m_X;
	vec1.m_Y -= vec2.m_Y;
	vec1.m_Z -= vec2.m_Z;
	vec1.m_W -= vec2.m_W;
    return vec1;
}

Vector4f& operator*= (Vector4f& vec1, const Vector4f& vec2)
{
	vec1.m_X *= vec2.m_X;
	vec1.m_Y *= vec2.m_Y;
	vec1.m_Z *= vec2.m_Z;
	vec1.m_W *= vec2.m_W;
    return vec1;
}

Vector4f& operator/= (Vector4f& vec1, const Vector4f& vec2)
{
	vec1.m_X /= vec2.m_X;
	vec1.m_Y /= vec2.m_Y;
	vec1.m_Z /= vec2.m_Z;
	vec1.m_W /= vec2.m_W;
    return vec1;
}

Vector4f& operator+= (Vector4f& vec, f32 scalar)
{
	vec.m_X += scalar;
	vec.m_Y += scalar;
	vec.m_Z += scalar;
	vec.m_W += scalar;
    return vec;
}

Vector4f& operator-= (Vector4f& vec, f32 scalar)
{
	vec.m_X -= scalar;
	vec.m_Y -= scalar;
	vec.m_Z -= scalar;
	vec.m_W -= scalar;
    return vec;
}

Vector4f& operator*= (Vector4f& vec, f32 scalar)
{
	vec.m_X *= scalar;
	vec.m_Y *= scalar;
	vec.m_Z *= scalar;
	vec.m_W *= scalar;
    return vec;
}

Vector4f& operator/= (Vector4f& vec, f32 scalar)
{
	f32 rcpScalar = Rcp(scalar);
	vec.m_X *= rcpScalar;
	vec.m_Y *= rcpScalar;
	vec.m_Z *= rcpScalar;
	vec.m_W *= rcpScalar;
    return vec;
}

const Vector4f operator+ (const Vector4f& vec1, const Vector4f& vec2)
{
    return Vector4f(vec1.m_X + vec2.m_X, vec1.m_Y + vec2.m_Y, vec1.m_Z + vec2.m_Z, vec1.m_W + vec2.m_W);
}

const Vector4f operator- (const Vector4f& vec1, const Vector4f& vec2)
{
	return Vector4f(vec1.m_X - vec2.m_X, vec1.m_Y - vec2.m_Y, vec1.m_Z - vec2.m_Z, vec1.m_W - vec2.m_W);
}

const Vector4f operator* (const Vector4f& vec1, const Vector4f& vec2)
{
	return Vector4f(vec1.m_X * vec2.m_X, vec1.m_Y * vec2.m_Y, vec1.m_Z * vec2.m_Z, vec1.m_W * vec2.m_W);
}

const Vector4f operator/ (const Vector4f& vec1, const Vector4f& vec2)
{
	return Vector4f(vec1.m_X / vec2.m_X, vec1.m_Y / vec2.m_Y, vec1.m_Z / vec2.m_Z, vec1.m_W / vec2.m_W);
}

const Vector4f operator+ (const Vector4f& vec, f32 scalar)
{
    return Vector4f(vec.m_X + scalar, vec.m_Y + scalar, vec.m_Z + scalar, vec.m_W + scalar);
}

const Vector4f operator+ (f32 scalar, const Vector4f& vec)
{
	return Vector4f(scalar + vec.m_X, scalar + vec.m_Y, scalar + vec.m_Z, scalar + vec.m_W);
}

const Vector4f operator- (const Vector4f& vec, f32 scalar)
{
    return Vector4f(vec.m_X - scalar, vec.m_Y - scalar, vec.m_Z - scalar, vec.m_W - scalar);
}

const Vector4f operator- (f32 scalar, const Vector4f& vec)
{
	return Vector4f(scalar - vec.m_X, scalar - vec.m_Y, scalar - vec.m_Z, scalar - vec.m_W);
}

const Vector4f operator* (const Vector4f& vec, f32 scalar)
{
	return Vector4f(vec.m_X * scalar, vec.m_Y * scalar, vec.m_Z * scalar, vec.m_W * scalar);
}

const Vector4f operator* (f32 scalar, const Vector4f& vec)
{
	return Vector4f(scalar * vec.m_X, scalar * vec.m_Y, scalar * vec.m_Z, scalar * vec.m_W);
}

const Vector4f operator/ (const Vector4f& vec, f32 scalar)
{
	f32 rcpScalar = Rcp(scalar);
    return Vector4f(vec.m_X * rcpScalar, vec.m_Y * rcpScalar, vec.m_Z * rcpScalar, vec.m_W * rcpScalar);
}

const Vector4f operator/ (f32 scalar, const Vector4f& vec)
{
	return Vector4f(scalar / vec.m_X, scalar / vec.m_Y, scalar / vec.m_Z, scalar / vec.m_W);
}

const Vector4i Vector4i::ONE(1, 1, 1, 1);
const Vector4i Vector4i::ZERO(0, 0, 0, 0);

Vector4i::Vector4i()
	: Vector4i(0)
{
}

Vector4i::Vector4i(i32 x, i32 y, i32 z, i32 w)
	: m_X(x)
	, m_Y(y)
	, m_Z(z)
	, m_W(w)
{
}

Vector4i::Vector4i(i32 scalar)
	: Vector4i(scalar, scalar, scalar, scalar)
{
}

i32& Vector4i::operator[] (u8 index)
{
	assert(index < 4);
	return *(&m_X + index);
}

const i32& Vector4i::operator[] (u8 index) const
{
	assert(index < 4);
	return *(&m_X + index);
}

const Vector4i Vector4i::operator- () const
{
	return Vector4i(-m_X, -m_Y, -m_Z, -m_W);
}

const Vector4u Vector4u::ONE(1, 1, 1, 1);
const Vector4u Vector4u::ZERO(0, 0, 0, 0);

Vector4u::Vector4u()
	: Vector4u(0)
{
}

Vector4u::Vector4u(u32 x, u32 y, u32 z, u32 w)
	: m_X(x)
	, m_Y(y)
	, m_Z(z)
	, m_W(w)
{
}

Vector4u::Vector4u(u32 scalar)
	: Vector4u(scalar, scalar, scalar, scalar)
{
}

u32& Vector4u::operator[] (u8 index)
{
	assert(index < 4);
	return *(&m_X + index);
}

const u32& Vector4u::operator[] (u8 index) const
{
	assert(index < 4);
	return *(&m_X + index);
}
