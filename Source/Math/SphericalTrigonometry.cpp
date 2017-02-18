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
	return 0.0f;
}
