#pragma once

#include "Math/Vector3.h"
#include "Math/Radian.h"

struct AxisAngle
{
	AxisAngle(const Vector3f& axis, const Radian& angle);

	Vector3f m_Axis;
	Radian m_Angle;
};
