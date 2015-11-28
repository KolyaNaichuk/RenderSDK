#pragma once

#include "Plane.h"

struct Matrix4f;
struct Vector3f;

enum FrustumPlane
{
    FrustumPlane_Left = 0,
    FrustumPlane_Right,
    FrustumPlane_Top,
    FrustumPlane_Bottom,
    FrustumPlane_Near,
    FrustumPlane_Far
};

struct BoundingFrustum
{
    BoundingFrustum(const Matrix4f& viewMatrix);
	Plane m_Planes[6];
}; 