#pragma once

#include "Vector3.h"

struct AxisAlignedBox
{
	AxisAlignedBox(const Vector3f& center, const Vector3f& radius);
	AxisAlignedBox(u32 numPoints, const Vector3f* pFirstPoint);
	AxisAlignedBox(const AxisAlignedBox& box1, const AxisAlignedBox& box2);
		    
    Vector3f m_Center;
    Vector3f m_Radius;
};
