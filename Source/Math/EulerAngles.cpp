#include "Math/EulerAngles.h"

EulerAngles::EulerAngles()
	: m_X(0.0f)
	, m_Y(0.0f)
	, m_Z(0.0f)
{
}

EulerAngles::EulerAngles(const Radian& x, const Radian& y, const Radian& z)
	: m_X(x)
	, m_Y(y)
	, m_Z(z)
{
}
