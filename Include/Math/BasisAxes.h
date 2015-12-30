#pragma once

#include "Math/Vector3f.h"

struct BasisAxes
{
	BasisAxes();
	BasisAxes(const Vector3f& xAxis, const Vector3f& yAxis, const Vector3f& zAxis);

	Vector3f m_XAxis;
	Vector3f m_YAxis;
	Vector3f m_ZAxis;
};