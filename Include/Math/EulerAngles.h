#pragma once

#include "Math/Radian.h"

struct EulerAngles
{
	EulerAngles();
	EulerAngles(const Radian& xAxisAngle, const Radian& yAxisAngle, const Radian& zAxisAngle);

	Radian m_XAxisAngle;
	Radian m_YAxisAngle;
	Radian m_ZAxisAngle;
};
