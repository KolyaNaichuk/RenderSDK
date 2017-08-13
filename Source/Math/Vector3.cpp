#include "Math/Vector3.h"
#include "Math/Transform.h"

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

f32& Vector3f::operator[] (u8 index)
{
	assert(index < 3);
	return *(&m_X + index);
}

const f32& Vector3f::operator[] (u8 index) const
{
	assert(index < 3);
	return *(&m_X + index);
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

bool AreEqual(const Vector3f& vec1, const Vector3f& vec2, f32 epsilon)
{
    return (::AreEqual(vec1.m_X, vec2.m_X, epsilon) &&
        ::AreEqual(vec1.m_Y, vec2.m_Y, epsilon) &&
        ::AreEqual(vec1.m_Z, vec2.m_Z, epsilon));
}

bool AreOrthogonal(const Vector3f& vec1, const Vector3f& vec2, f32 epsilon)
{
	return ::AreEqual(Dot(vec1, vec2), 0.0f, epsilon);
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

const Vector3f TransformPoint(const Vector3f& point, const Matrix4f& matrix)
{
	f32 x = point.m_X * matrix.m_00 + point.m_Y * matrix.m_10 + point.m_Z * matrix.m_20 + matrix.m_30;
	f32 y = point.m_X * matrix.m_01 + point.m_Y * matrix.m_11 + point.m_Z * matrix.m_21 + matrix.m_31;
	f32 z = point.m_X * matrix.m_02 + point.m_Y * matrix.m_12 + point.m_Z * matrix.m_22 + matrix.m_32;
	f32 w = point.m_X * matrix.m_03 + point.m_Y * matrix.m_13 + point.m_Z * matrix.m_23 + matrix.m_33;

	f32 rcpW = 1.0f / w;
	return Vector3f(x * rcpW, y * rcpW, z * rcpW);
}

const Vector3f TransformPoint(const Vector3f& point, const Transform& transform)
{
	return TransformPoint(point, transform.GetLocalToWorldMatrix());
}

Vector3f ToCartesianVector(const Vector4f& homogeneousVec)
{
	return Vector3f(homogeneousVec.m_X, homogeneousVec.m_Y, homogeneousVec.m_Z);
}

Vector3f ToCartesianPoint(const Vector4f& homogeneousPoint)
{
	f32 rcpW = 1.0f / homogeneousPoint.m_W;
	return Vector3f(homogeneousPoint.m_X * rcpW, homogeneousPoint.m_Y * rcpW, homogeneousPoint.m_Z * rcpW);
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
	f32 rcpScalar = Rcp(scalar);
	vec.m_X *= rcpScalar;
	vec.m_Y *= rcpScalar;
	vec.m_Z *= rcpScalar;
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
	f32 rcpScalar = Rcp(scalar);
    return Vector3f(vec.m_X * rcpScalar, vec.m_Y * rcpScalar, vec.m_Z * rcpScalar);
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

i32& Vector3i::operator[] (u8 index)
{
	assert(index < 3);
	return *(&m_X + index);
}

const i32& Vector3i::operator[] (u8 index) const
{
	assert(index < 3);
	return *(&m_X + index);
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

u32& Vector3u::operator[] (u8 index)
{
	assert(index < 3);
	return *(&m_X + index);
}

const u32& Vector3u::operator[] (u8 index) const
{
	assert(index < 3);
	return *(&m_X + index);
}