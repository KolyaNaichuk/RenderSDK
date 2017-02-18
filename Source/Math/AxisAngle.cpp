#include "Math/AxisAngle.h"

AxisAngle::AxisAngle(const Vector3f& axis, f32 angleInRadians)
	: m_Axis(axis)
	, m_AngleInRadians(angleInRadians)
{
}
