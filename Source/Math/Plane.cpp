#include "Math/Plane.h"
#include "Math/Sphere.h"
#include "Math/Transform.h"
#include "Math/Vector4.h"

Plane::Plane()
	: m_Normal(Vector3f::ZERO)
	, m_SignedDistFromOrigin(0.0f)
{
}

Plane::Plane(const Vector3f& normal, f32 signedDistFromOrigin)
	: m_Normal(normal)
	, m_SignedDistFromOrigin(signedDistFromOrigin)
{
}

Plane::Plane(const Vector3f& point, const Vector3f& normal)
	: m_Normal(normal)
	, m_SignedDistFromOrigin(-Dot(m_Normal, point))
{
}

Plane::Plane(const Vector3f& point1, const Vector3f& point2, const Vector3f& point3)
	: m_Normal(Cross(point2 - point1, point3 - point1))
	, m_SignedDistFromOrigin(-Dot(m_Normal, point1))
{
}

Plane::Plane(const Vector4f& planeEquationCoeffs)
	: m_Normal(planeEquationCoeffs.m_X, planeEquationCoeffs.m_Y, planeEquationCoeffs.m_Z)
	, m_SignedDistFromOrigin(planeEquationCoeffs.m_W)
{
}

const Plane Normalize(const Plane& plane)
{
	const f32 rcpLength = Rcp(Length(plane.m_Normal));
	return Plane(rcpLength * plane.m_Normal, rcpLength * plane.m_SignedDistFromOrigin);
}

f32 SignedDistanceToPoint(const Plane& plane, const Vector3f& point)
{
	return (Dot(point, plane.m_Normal) + plane.m_SignedDistFromOrigin);
}

Plane::HalfSpace ClassifyPoint(const Plane& plane, const Vector3f& point)
{
	const f32 signedDist = SignedDistanceToPoint(plane, point);	
	if (signedDist > 0.0f)
		return Plane::Front;
	if (signedDist < 0.0f)
		return Plane::Back;
	return Plane::On;
}

Plane::HalfSpace ClassifySphere(const Plane& plane, const Sphere& sphere)
{
	const f32 signedDist = SignedDistanceToPoint(plane, sphere.m_Center);
	if (signedDist > sphere.m_Radius)
		return Plane::Front;
	if (signedDist < -sphere.m_Radius)
		return Plane::Back;
	return Plane::On;
}

bool CalcIntersectionPoint(Vector3f* pOutPoint, const Plane& plane1, const Plane& plane2, const Plane& plane3)
{
	// Based on chapter 5.4.5 Intersection of Three Planes from Real-Time Collision Detection book

	assert(pOutPoint != nullptr);
	Vector3f u = Cross(plane2.m_Normal, plane3.m_Normal);
	
	f32 denom = Dot(plane1.m_Normal, u);
	if (Abs(denom) < EPSILON)
		return false;
		
	*pOutPoint = Cross(plane1.m_Normal, plane2.m_SignedDistFromOrigin * plane3.m_Normal - plane3.m_SignedDistFromOrigin * plane2.m_Normal);
	*pOutPoint -= plane1.m_SignedDistFromOrigin * u;
	*pOutPoint /= denom;

	return true;
}
