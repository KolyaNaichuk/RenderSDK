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

bool IsOrthonormal(const BasisAxes& basis, f32 epsilon)
{
	return IsNormalized(basis.m_XAxis, epsilon) &&
		IsNormalized(basis.m_YAxis, epsilon) &&
		IsNormalized(basis.m_ZAxis, epsilon) &&
		AreOrthogonal(basis.m_XAxis, basis.m_YAxis, epsilon) &&
		AreOrthogonal(basis.m_XAxis, basis.m_ZAxis, epsilon) &&
		AreOrthogonal(basis.m_YAxis, basis.m_ZAxis, epsilon);
}