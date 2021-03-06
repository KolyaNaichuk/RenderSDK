#pragma once

#include "Math/Vector3.h"

struct Matrix4f;

struct BasisAxes
{
	BasisAxes();
	BasisAxes(const Vector3f& xAxis, const Vector3f& yAxis, const Vector3f& zAxis);
	BasisAxes(const Matrix4f& rotationMatrix);

	Vector3f m_XAxis;
	Vector3f m_YAxis;
	Vector3f m_ZAxis;
};

bool IsOrthonormal(const BasisAxes& basis, f32 epsilon = EPSILON);