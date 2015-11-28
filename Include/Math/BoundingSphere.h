#pragma once

#include "Vector3f.h"

struct BoundingSphere
{
    BoundingSphere(const Vector3f& center, f32 radius);
	BoundingSphere(u32 numPoints, const Vector3f* pFirstPoint);
    BoundingSphere(const BoundingSphere& sphere1, const BoundingSphere& sphere2);
            
    Vector3f m_Center;
    f32 m_Radius;
};