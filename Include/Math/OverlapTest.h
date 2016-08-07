#pragma once

struct AxisAlignedBox;
struct Sphere;
struct Plane;
struct Frustum;

bool Overlap(const AxisAlignedBox& box1, const AxisAlignedBox& box2);
bool Overlap(const Sphere& sphere1, const Sphere& sphere2);
bool TestAABBAgainstPlane(const Plane& plane, const AxisAlignedBox& box);
bool TestAABBAgainstFrustum(const Frustum& frustum, const AxisAlignedBox& box);