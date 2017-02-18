#pragma once

#include "Math/Math.h"

struct Vector3f;
struct Sphere;
struct Triangle;
struct Quad;

f32 DistanceOnSphere(const Sphere& sphere, const Vector3f& point1, const Vector3f& point2);
f32 AreaOnSphere(const Sphere& sphere, const Triangle& triangle);
f32 AreaOnSphere(const Sphere& sphere, const Quad& quad);