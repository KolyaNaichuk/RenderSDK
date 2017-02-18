#pragma once

#include "Math/Vector3.h"

struct Sphere;
struct Triangle;
struct Quad;

f32 DistanceOnSphere(const Sphere& sphere, const Vector3f& point1, const Vector3f& point2);

f32 AreaOnSphere(const Sphere& sphere, const Triangle& triangle);
f32 AreaOnSphere(const Sphere& sphere, const Quad& quad);

f32 SolidAngle(const Sphere& sphere, const Triangle& triangle);
f32 SolidAngle(const Sphere& sphere, const Quad& quad);

const Vector3f ProjectOntoSphere(const Sphere& sphere, const Vector3f& point);