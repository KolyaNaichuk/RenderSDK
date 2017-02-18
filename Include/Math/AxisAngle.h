#pragma once

#include "Math/Vector3.h"

struct AxisAngle
{
	AxisAngle(const Vector3f& axis, f32 angleInRadians);

	Vector3f m_Axis;
	f32 m_AngleInRadians;
};
