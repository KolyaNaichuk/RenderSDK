#include "Math/Plane.h"
#include "Math/Transform.h"

Plane::Plane()
	: m_Normal(Vector3f::ZERO)
	, m_SignedDistFromOrigin(0.0f)
{
}

Plane::Plane(const Vector3f& point1, const Vector3f& point2, const Vector3f& point3)
	: m_Normal(Cross(point2 - point1, point3 - point1))
	, m_SignedDistFromOrigin(-Dot(m_Normal, point1))
{
}

Plane::Plane(const Vector3f& point, const Vector3f& normal)
	: m_Normal(normal)
	, m_SignedDistFromOrigin(-Dot(m_Normal, point))
{
}

Plane::Plane(const Vector3f& normal, f32 signedDistFromOrigin)
	: m_Normal(normal)
	, m_SignedDistFromOrigin(signedDistFromOrigin)
{
}

const Plane Normalize(const Plane& plane)
{
	f32 rcpLength = Rcp(Length(plane.m_Normal));
	return Plane(rcpLength * plane.m_Normal, rcpLength * plane.m_SignedDistFromOrigin);
}

bool IsNormalized(const Plane& plane, f32 epsilon)
{
	return IsNormalized(plane.m_Normal, epsilon);
}

f32 SignedDistanceToPoint(const Plane& plane, const Vector3f& point)
{
	assert(IsNormalized(plane));
	return (Dot(point, plane.m_Normal) + plane.m_SignedDistFromOrigin);
}

Plane::HalfSpace ClassifyPoint(const Plane& plane, const Vector3f& point)
{
	f32 signedDist = Dot(point, plane.m_Normal) + plane.m_SignedDistFromOrigin;	
	
	if (signedDist > 0.0f)
		return Plane::Front;
	
	if (signedDist < 0.0f)
		return Plane::Back;

	return Plane::OnPlane;
}

const Plane TransformPlane(const Plane& plane, const Transform& transform)
{
	Vector4f vec(plane.m_Normal.m_X, plane.m_Normal.m_Y, plane.m_Normal.m_Z, plane.m_SignedDistFromOrigin);
	Vector4f transformedVec = TransformNormal(vec, transform);
	return Plane(Vector3f(transformedVec.m_X, transformedVec.m_Y, transformedVec.m_Z), transformedVec.m_W);
}

const Vector4f ToVector4f(const Plane& plane)
{
	return Vector4f(plane.m_Normal.m_X, plane.m_Normal.m_Y, plane.m_Normal.m_Z, plane.m_SignedDistFromOrigin);
}
