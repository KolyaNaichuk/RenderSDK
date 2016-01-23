#include "Math/Frustum.h"
#include "Math/Transform.h"
#include "Math/Vector4.h"

Frustum::Frustum(const Matrix4f& projMatrix)
{
	Vector4f clipSpaceCorners[NumCorners];
	
	clipSpaceCorners[NearTopLeft] = Vector4f(-1.0f, 1.0f, 0.0f, 1.0f);
	clipSpaceCorners[NearTopRight] = Vector4f(1.0f, 1.0f, 0.0f, 1.0f);
	clipSpaceCorners[NearBottomLeft] = Vector4f(-1.0f, -1.0f, 0.0f, 1.0f);
	clipSpaceCorners[NearBottomRight] = Vector4f(1.0f, -1.0f, 0.0f, 1.0f);
	
	clipSpaceCorners[FarTopLeft] = Vector4f(-1.0f, 1.0f, 1.0f, 1.0f);
	clipSpaceCorners[FarTopRight] = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
	clipSpaceCorners[FarBottomLeft] = Vector4f(-1.0f, -1.0f, 1.0f, 1.0f);
	clipSpaceCorners[FarBottomRight] = Vector4f(1.0f, -1.0f, 1.0f, 1.0f);

	Matrix4f invProjMatrix = Inverse(projMatrix);
	for (u8 cornerIndex = 0; cornerIndex < NumCorners; ++cornerIndex)
	{
		Vector4f objectSpaceCorner = clipSpaceCorners[cornerIndex] * invProjMatrix;
		objectSpaceCorner /= objectSpaceCorner.m_W;
		
		m_Corners[cornerIndex] = Vector3f(objectSpaceCorner.m_X, objectSpaceCorner.m_Y, objectSpaceCorner.m_Z);
	}

	m_Planes[NearPlane].m_Normal = Vector3f(projMatrix.m_02, projMatrix.m_12, projMatrix.m_22);
	m_Planes[NearPlane].m_SignedDistFromOrigin = projMatrix.m_32;
	m_Planes[NearPlane] = Normalize(m_Planes[NearPlane]);

	m_Planes[FarPlane].m_Normal = Vector3f(projMatrix.m_03 - projMatrix.m_02, projMatrix.m_13 - projMatrix.m_12, projMatrix.m_23 - projMatrix.m_22);
	m_Planes[FarPlane].m_SignedDistFromOrigin = projMatrix.m_33 - projMatrix.m_32;
	m_Planes[FarPlane] = Normalize(m_Planes[FarPlane]);

	m_Planes[LeftPlane].m_Normal = Vector3f(projMatrix.m_03 + projMatrix.m_00, projMatrix.m_13 + projMatrix.m_10, projMatrix.m_23 + projMatrix.m_20);
	m_Planes[LeftPlane].m_SignedDistFromOrigin = projMatrix.m_33 + projMatrix.m_30;
	m_Planes[LeftPlane] = Normalize(m_Planes[LeftPlane]);

	m_Planes[RightPlane].m_Normal = Vector3f(projMatrix.m_03 - projMatrix.m_00, projMatrix.m_13 - projMatrix.m_10, projMatrix.m_23 - projMatrix.m_20);
	m_Planes[RightPlane].m_SignedDistFromOrigin = projMatrix.m_33 - projMatrix.m_30;
	m_Planes[RightPlane] = Normalize(m_Planes[RightPlane]);
	
	m_Planes[TopPlane].m_Normal = Vector3f(projMatrix.m_03 - projMatrix.m_01, projMatrix.m_13 - projMatrix.m_11, projMatrix.m_23 - projMatrix.m_21);
	m_Planes[TopPlane].m_SignedDistFromOrigin = projMatrix.m_33 - projMatrix.m_31;
	m_Planes[TopPlane] = Normalize(m_Planes[TopPlane]);

	m_Planes[BottomPlane].m_Normal = Vector3f(projMatrix.m_03 + projMatrix.m_01, projMatrix.m_13 + projMatrix.m_11, projMatrix.m_23 + projMatrix.m_21);
	m_Planes[BottomPlane].m_SignedDistFromOrigin = projMatrix.m_33 + projMatrix.m_31;
	m_Planes[BottomPlane] = Normalize(m_Planes[BottomPlane]);
}

const Frustum TransformFrustum(Frustum frustum, const Transform& transform)
{
	for (u8 cornerIndex = 0; cornerIndex < Frustum::NumCorners; ++cornerIndex)
		frustum.m_Corners[cornerIndex] = TransformPoint(frustum.m_Corners[cornerIndex], transform);

	for (u8 planeIndex = 0; planeIndex < Frustum::NumPlanes; ++planeIndex)
		frustum.m_Planes[planeIndex] = TransformPlane(frustum.m_Planes[planeIndex], transform);

	return frustum;
}
