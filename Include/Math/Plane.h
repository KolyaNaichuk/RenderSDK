#pragma once

#include "Math/Vector3.h"
#include "Math/Vector4.h"

class Transform;

struct Plane
{
	enum HalfSpace
	{
		Front,
		Back,
		OnPlane
	};

	Plane();
	Plane(const Vector3f& point1, const Vector3f& point2, const Vector3f& point3);
	Plane(const Vector3f& point, const Vector3f& normal);
	Plane(const Vector3f& normal, f32 signedDistFromOrigin);

    Vector3f m_Normal;
	f32 m_SignedDistFromOrigin;
};

const Plane Normalize(const Plane& plane);
bool IsNormalized(const Plane& plane, f32 epsilon = EPSILON);
f32 SignedDistanceToPoint(const Plane& plane, const Vector3f& point);
Plane::HalfSpace ClassifyPoint(const Plane& plane, const Vector3f& point);
const Plane TransformPlane(const Plane& plane, const Transform& transform);
const Vector4f ToVector4f(const Plane& plane);
