#include "Math/Frustum.h"
#include "Math/Matrix4.h"

Frustum::Frustum()
{
}

Frustum::Frustum(const Matrix4f& transformMatrix)
{
	// NB: the normal vectors of the resulting planes are pointing inside of the frustum

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

	CalcIntersectionPoint(&m_Corners[NearTopLeft], m_Planes[NearPlane], m_Planes[TopPlane], m_Planes[LeftPlane]);
	CalcIntersectionPoint(&m_Corners[NearTopRight], m_Planes[NearPlane], m_Planes[TopPlane], m_Planes[RightPlane]);
	CalcIntersectionPoint(&m_Corners[NearBottomLeft], m_Planes[NearPlane], m_Planes[BottomPlane], m_Planes[LeftPlane]);
	CalcIntersectionPoint(&m_Corners[NearBottomRight], m_Planes[NearPlane], m_Planes[BottomPlane], m_Planes[RightPlane]);
	CalcIntersectionPoint(&m_Corners[FarTopLeft], m_Planes[FarPlane], m_Planes[TopPlane], m_Planes[LeftPlane]);
	CalcIntersectionPoint(&m_Corners[FarTopRight], m_Planes[FarPlane], m_Planes[TopPlane], m_Planes[RightPlane]);
	CalcIntersectionPoint(&m_Corners[FarBottomLeft], m_Planes[FarPlane], m_Planes[BottomPlane], m_Planes[LeftPlane]);
	CalcIntersectionPoint(&m_Corners[FarBottomRight], m_Planes[FarPlane], m_Planes[BottomPlane], m_Planes[RightPlane]);
}