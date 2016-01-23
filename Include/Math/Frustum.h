#pragma once

#include "Math/Vector3.h"
#include "Math/Plane.h"

struct Matrix4f;
struct Quaternion;
class Transform;

struct Frustum
{
	enum Corners
	{
		NearTopLeft = 0,
		NearTopRight,
		NearBottomLeft,
		NearBottomRight,
		FarTopLeft,
		FarTopRight,
		FarBottomLeft,
		FarBottomRight,
		NumCorners 
	};

	enum Planes
	{
		NearPlane = 0,
		FarPlane,
		LeftPlane,
		RightPlane,
		TopPlane,
		BottomPlane,
		NumPlanes
	};

	Frustum(const Matrix4f& projMatrix);
	
	Vector3f m_Corners[NumCorners];
	Plane m_Planes[NumPlanes];	
};

const Frustum TransformFrustum(Frustum frustum, const Transform& transform);
