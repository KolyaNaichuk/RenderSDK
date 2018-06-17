#include "Math/BasisAxes.h"
#include "Math/Matrix4.h"
#include "Math/Transform.h"

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

BasisAxes::BasisAxes(const Matrix4f& rotationMatrix)
	: m_XAxis(rotationMatrix.m_00, rotationMatrix.m_01, rotationMatrix.m_02)
	, m_YAxis(rotationMatrix.m_10, rotationMatrix.m_11, rotationMatrix.m_12)
	, m_ZAxis(rotationMatrix.m_20, rotationMatrix.m_21, rotationMatrix.m_22)
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