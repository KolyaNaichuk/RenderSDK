#pragma once

#include "Math/Vector3.h"

struct Quad
{
	Quad(const Vector3f& point1, const Vector3f& point2, const Vector3f& point3, const Vector3f& point4);

	Vector3f m_Point1;
	Vector3f m_Point2;
	Vector3f m_Point3;
	Vector3f m_Point4;
};

