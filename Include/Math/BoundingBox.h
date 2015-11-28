#pragma once

#include "Vector3f.h"

struct BoundingBox
{
	BoundingBox(const Vector3f& center, const Vector3f& radius);
	BoundingBox(u32 numPoints, const Vector3f* pFirstPoint);
    BoundingBox(const BoundingBox& box1, const BoundingBox& box2);
		    
    Vector3f m_Center;
    Vector3f m_Radius;
};
