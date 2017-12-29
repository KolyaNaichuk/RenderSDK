#include "Math/Frustum.h"
#include "Math/Matrix4.h"

Frustum::Frustum(const Matrix4f& transformMatrix)
{
	const Vector4f col0(transformMatrix.m_00, transformMatrix.m_10, transformMatrix.m_20, transformMatrix.m_30);
	const Vector4f col1(transformMatrix.m_01, transformMatrix.m_11, transformMatrix.m_21, transformMatrix.m_31);
	const Vector4f col2(transformMatrix.m_02, transformMatrix.m_12, transformMatrix.m_22, transformMatrix.m_32);
	const Vector4f col3(transformMatrix.m_03, transformMatrix.m_13, transformMatrix.m_23, transformMatrix.m_33);
	
	m_Planes[NearPlane] = Normalize(Plane(col2));
	m_Planes[FarPlane] = Normalize(Plane(col3 - col2));
	m_Planes[LeftPlane] = Normalize(Plane(col3 + col0));
	m_Planes[RightPlane] = Normalize(Plane(col3 - col0));
	m_Planes[TopPlane] = Normalize(Plane(col3 - col1));
	m_Planes[BottomPlane] = Normalize(Plane(col3 + col1));
}






