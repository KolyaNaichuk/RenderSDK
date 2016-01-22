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
	for (i8 index = 0; index < NumCorners; ++index)
	{
		Vector4f objectSpaceCorner = clipSpaceCorners[index] * invProjMatrix;
		objectSpaceCorner /= objectSpaceCorner.m_W;
		
		m_Corners[index] = Vector3f(objectSpaceCorner.m_X, objectSpaceCorner.m_Y, objectSpaceCorner.m_Z);
	}
}
