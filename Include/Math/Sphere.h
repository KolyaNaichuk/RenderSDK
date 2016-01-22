#pragma once

#include "Vector3.h"

struct Sphere
{
	Sphere(const Vector3f& center, f32 radius);
	Sphere(u32 numPoints, const Vector3f* pFirstPoint);
	Sphere(const Sphere& sphere1, const Sphere& sphere2);
            
    Vector3f m_Center;
    f32 m_Radius;
};