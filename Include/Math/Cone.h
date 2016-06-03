#pragma once

#include "Math/Vector3.h"
#include "Math/Radian.h"

struct Cone
{
	Cone(const Vector3f& apexPoint, const Radian& apexAngle, const Vector3f& axisDir, f32 height);

	Vector3f m_ApexPoint;
	Radian m_ApexAngle;
	Vector3f m_AxisDir;
	f32 m_Height;
};
