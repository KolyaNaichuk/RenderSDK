#include "Math/BasisAxes.h"

BasisAxes::BasisAxes()
	: BasisAxes(Vector3f::RIGHT, Vector3f::UP, Vector3f::FORWARD)
{
}

BasisAxes::BasisAxes(const Vector3f& xAxis, const Vector3f& yAxis, const Vector3f& zAxis)
	: m_XAxis(xAxis)
	, m_YAxis(yAxis)
	, m_ZAxis(zAxis)
{
}
