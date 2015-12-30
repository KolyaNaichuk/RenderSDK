#include "Math/EulerAngles.h"

EulerAngles::EulerAngles()
	: m_ZAxisAngle(0.0f)
	, m_XAxisAngle(0.0f)
	, m_YAxisAngle(0.0f)
{
}

EulerAngles::EulerAngles(const Radian& zAxisAngle, const Radian& xAxisAngle, const Radian& yAxisAngle)
	: m_ZAxisAngle(zAxisAngle)
	, m_XAxisAngle(xAxisAngle)
	, m_YAxisAngle(yAxisAngle)
{
}

