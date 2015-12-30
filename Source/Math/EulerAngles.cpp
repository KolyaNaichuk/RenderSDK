#include "Math/EulerAngles.h"

EulerAngles::EulerAngles()
	: m_XAxisAngle(0.0f)
	, m_YAxisAngle(0.0f)
	, m_ZAxisAngle(0.0f)
{
}

EulerAngles::EulerAngles(const Radian& xAxisAngle, const Radian& yAxisAngle, const Radian& zAxisAngle)
	: m_XAxisAngle(xAxisAngle)
	, m_YAxisAngle(yAxisAngle)
	, m_ZAxisAngle(zAxisAngle)
{
}

