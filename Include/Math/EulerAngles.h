#pragma once

#include "Math/Radian.h"

struct EulerAngles
{
	EulerAngles();
	EulerAngles(const Radian& zAxisAngle, const Radian& xAxisAngle, const Radian& yAxisAngle);

	Radian m_ZAxisAngle;
	Radian m_XAxisAngle;
	Radian m_YAxisAngle;
};
