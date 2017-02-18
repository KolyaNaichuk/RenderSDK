#include "Math/EulerAngles.h"

EulerAngles::EulerAngles()
	: EulerAngles(0.0f, 0.0f, 0.0f)
{
}

EulerAngles::EulerAngles(f32 zAxisAngleInRadians, f32 xAxisAngleInRadians, f32 yAxisAngleInRadians)
	: m_ZAxisAngleInRadians(zAxisAngleInRadians)
	, m_XAxisAngleInRadians(xAxisAngleInRadians)
	, m_YAxisAngleInRadians(yAxisAngleInRadians)
{
}

