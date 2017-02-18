#pragma once

#include "Math/Vector3.h"

struct Triangle
{
	Triangle(const Vector3f& point1, const Vector3f& point2, const Vector3f& point3);

	Vector3f m_Point1;
	Vector3f m_Point2;
	Vector3f m_Point3;
};