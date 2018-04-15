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