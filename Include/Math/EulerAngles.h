#pragma once

#include "Math/Math.h"

struct EulerAngles
{
	EulerAngles();
	EulerAngles(f32 zAxisAngleInRadians, f32 xAxisAngleInRadians, f32 yAxisAngleInRadians);

	f32 m_ZAxisAngleInRadians;
	f32 m_XAxisAngleInRadians;
	f32 m_YAxisAngleInRadians;
};
