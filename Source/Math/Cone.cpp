#include "Math/Cone.h"

Cone::Cone(const Vector3f& apexPoint, const Radian& apexAngle, const Vector3f& axisDir, f32 height)
	: m_ApexPoint(apexPoint)
	, m_ApexAngle(apexAngle)
	, m_AxisDir(axisDir)
	, m_Height(height)
{
	assert(IsNormalized(m_AxisDir));
}
