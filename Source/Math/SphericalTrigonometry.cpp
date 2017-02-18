#include "Math/SphericalTrigonometry.h"
#include "Math/Vector3.h"
#include "Math/Sphere.h"
#include "Math/Triangle.h"
#include "Math/Quad.h"

f32 DistanceOnSphere(const Sphere& sphere, const Vector3f& point1, const Vector3f& point2)
{
	Vector3f normal1 = Normalize(point1 - sphere.m_Center);
	Vector3f normal2 = Normalize(point2 - sphere.m_Center);

	f32 angleInRadians = ArcCos(Dot(normal1, normal2));
	return angleInRadians * sphere.m_Radius;
}

f32 AreaOnSphere(const Sphere& sphere, const Triangle& triangle)
{
	Vector3f normal1 = Normalize(triangle.m_Point1 - sphere.m_Center);
	Vector3f normal2 = Normalize(triangle.m_Point2 - sphere.m_Center);
	Vector3f normal3 = Normalize(triangle.m_Point3 - sphere.m_Center);

	f32 angleInRadians12 = ArcCos(Dot(normal1, normal2));
	f32 angleInRadians13 = ArcCos(Dot(normal1, normal3));
	f32 angleInRadians23 = ArcCos(Dot(normal2, normal3));

	f32 dist12 = angleInRadians12 * sphere.m_Radius;
	f32 dist13 = angleInRadians13 * sphere.m_Radius;
	f32 dist23 = angleInRadians23 * sphere.m_Radius;

	f32 halfPerim = 0.5f * (dist12 + dist13 + dist23);
	f32 sphericalExcess = 4.0f * ArcTan(Sqrt(Tan(0.5f * halfPerim) * Tan(0.5f * (halfPerim - dist12)) * Tan(0.5f * (halfPerim - dist13)) * Tan(0.5f * (halfPerim - dist23))));
	
	return sphericalExcess * Sqr(sphere.m_Radius);
}

f32 AreaOnSphere(const Sphere& sphere, const Quad& quad)
{
	Vector3f normal1 = Normalize(quad.m_Point1 - sphere.m_Center);
	Vector3f normal2 = Normalize(quad.m_Point2 - sphere.m_Center);
	Vector3f normal3 = Normalize(quad.m_Point3 - sphere.m_Center);
	Vector3f normal4 = Normalize(quad.m_Point4 - sphere.m_Center);

	f32 angleInRadians12 = ArcCos(Dot(normal1, normal2));
	f32 angleInRadians23 = ArcCos(Dot(normal2, normal3));
	f32 angleInRadians34 = ArcCos(Dot(normal3, normal4));
	f32 angleInRadians41 = ArcCos(Dot(normal4, normal1));
	f32 angleInRadians13 = ArcCos(Dot(normal1, normal3));

	f32 dist12 = angleInRadians12 * sphere.m_Radius;
	f32 dist23 = angleInRadians23 * sphere.m_Radius;
	f32 dist34 = angleInRadians34 * sphere.m_Radius;
	f32 dist41 = angleInRadians41 * sphere.m_Radius;
	f32 dist13 = angleInRadians13 * sphere.m_Radius;
	
	f32 halfPerim1 = 0.5f * (dist12 + dist23 + dist13);
	f32 quarterOfSphericalExcess1 = ArcTan(Sqrt(Tan(0.5f * halfPerim1) * Tan(0.5f * (halfPerim1 - dist12)) * Tan(0.5f * (halfPerim1 - dist23)) * Tan(0.5f * (halfPerim1 - dist13))));
	
	f32 halfPerim2 = 0.5f * (dist34 + dist41 + dist13);
	f32 quarterOfSphericalExcess2 = ArcTan(Sqrt(Tan(0.5f * halfPerim2) * Tan(0.5f * (halfPerim2 - dist34)) * Tan(0.5f * (halfPerim2 - dist41)) * Tan(0.5f * (halfPerim2 - dist13))));
	
	return 4.0f * (quarterOfSphericalExcess1 + quarterOfSphericalExcess2) * Sqr(sphere.m_Radius);
}

f32 SolidAngle(const Sphere& sphere, const Triangle& triangle)
{
	Sphere unitSphere(sphere.m_Center, 1.0f);
	
	Triangle projectedTriangle(ProjectOntoSphere(unitSphere, triangle.m_Point1),
		ProjectOntoSphere(unitSphere, triangle.m_Point2),
		ProjectOntoSphere(unitSphere, triangle.m_Point3));

	return AreaOnSphere(unitSphere, projectedTriangle);
}

f32 SolidAngle(const Sphere& sphere, const Quad& quad)
{
	Sphere unitSphere(sphere.m_Center, 1.0f);
	
	Quad projectedQuad(ProjectOntoSphere(unitSphere, quad.m_Point1),
		ProjectOntoSphere(unitSphere, quad.m_Point2),
		ProjectOntoSphere(unitSphere, quad.m_Point3),
		ProjectOntoSphere(unitSphere, quad.m_Point4));
		
	return AreaOnSphere(unitSphere, projectedQuad);
}

const Vector3f ProjectOntoSphere(const Sphere& sphere, const Vector3f& point)
{
	Vector3f dirToPoint = Normalize(point - sphere.m_Center);
	return (sphere.m_Center + sphere.m_Radius * dirToPoint);
}
