#include "Math/Cone.h"

Cone::Cone(const Vector3f& apexPoint, const Radian& apexAngle, const Vector3f& direction, f32 height)
	: m_ApexPoint(apexPoint)
	, m_ApexAngle(apexAngle)
	, m_Direction(direction)
	, m_Height(height)
{
	assert(IsNormalized(m_Direction));
}

const Sphere ExtractBoundingSphere(const Cone& cone)
{
	f32 sphereRadius = 0.5f * cone.m_Height / Sqr(Cos(0.5f * cone.m_ApexAngle));
	Vector3f sphereCenter = cone.m_ApexPoint + sphereRadius * cone.m_Direction;

	return Sphere(sphereCenter, sphereRadius);
}
