#pragma once

#include "Math/Sphere.h"
#include "Math/Radian.h"

struct Cone
{
	Cone(const Vector3f& apexPoint, const Radian& apexAngle, const Vector3f& direction, f32 height);

	Vector3f m_ApexPoint;
	Radian m_ApexAngle;
	Vector3f m_Direction;
	f32 m_Height;
};

const Sphere ExtractBoundingSphere(const Cone& cone);
