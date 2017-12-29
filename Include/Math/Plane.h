#pragma once

#include "Math/Vector3.h"

struct Vector4f;
struct Sphere;
class Transform;

struct Plane
{
	enum HalfSpace
	{
		Front,
		Back,
		On
	};

	Plane();
	Plane(const Vector3f& normal, f32 signedDistFromOrigin);
	Plane(const Vector3f& point, const Vector3f& normal);
	Plane(const Vector3f& point1, const Vector3f& point2, const Vector3f& point3);
	Plane(const Vector4f& planeEquationCoeffs);
	
    Vector3f m_Normal;
	f32 m_SignedDistFromOrigin;
};

const Plane Normalize(const Plane& plane);
f32 SignedDistanceToPoint(const Plane& plane, const Vector3f& point);
Plane::HalfSpace ClassifyPoint(const Plane& plane, const Vector3f& point);
Plane::HalfSpace ClassifySphere(const Plane& plane, const Sphere& sphere);