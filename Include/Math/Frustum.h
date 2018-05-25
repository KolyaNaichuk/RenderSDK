#pragma once

#include "Math/Plane.h"

struct Matrix4f;

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

	Frustum();
	Frustum(const Matrix4f& transformMatrix);

	Plane m_Planes[NumPlanes];	
	Vector3f m_Corners[NumCorners];
};