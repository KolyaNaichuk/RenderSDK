#pragma once

#include "Math/Radian.h"

struct EulerAngles
{
	EulerAngles();
	EulerAngles(const Radian& x, const Radian& y, const Radian& z);

	Radian m_X;
	Radian m_Y;
	Radian m_Z;
};

const EulerAngles Negate(const EulerAngles& eulerAngles);