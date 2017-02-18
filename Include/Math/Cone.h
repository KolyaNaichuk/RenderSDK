#pragma once

#include "Math/Sphere.h"

struct Cone
{
	Cone(const Vector3f& apexPoint, f32 apexAngleInRadians, const Vector3f& direction, f32 height);

	Vector3f m_ApexPoint;
	f32 m_ApexAngleInRadians;
	Vector3f m_Direction;
	f32 m_Height;
};

const Sphere ExtractBoundingSphere(const Cone& cone);
