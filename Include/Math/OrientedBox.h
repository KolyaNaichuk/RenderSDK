#pragma once

#include "Math/BasisAxes.h"

struct OrientedBox
{
	OrientedBox(const Vector3f& center, const BasisAxes& orientation, const Vector3f& radius);
	OrientedBox(u32 numPoints, const Vector3f* pFirstPoint);

	Vector3f m_Center;
	BasisAxes m_Orientation;
	Vector3f m_Radius;
};