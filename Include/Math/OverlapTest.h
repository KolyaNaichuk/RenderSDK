#pragma once

struct AxisAlignedBox;
struct Sphere;
struct Plane;
struct Frustum;
struct Vector3f;

bool Overlap(const AxisAlignedBox& box1, const AxisAlignedBox& box2);
bool Overlap(const Sphere& sphere1, const Sphere& sphere2);

bool TestAABBAgainstPlane(const Plane& plane, const AxisAlignedBox& box);
bool TestSphereAgainstPlane(const Plane& plane, const Sphere& sphere);
bool TestPointAgainstPlane(const Plane& plane, const Vector3f& point);

bool TestFrustumAgainstFrustum(const Frustum& frustum1, const Frustum& frustum2);
bool TestAABBAgainstFrustum(const Frustum& frustum, const AxisAlignedBox& box);
bool TestSphereAgainstFrustum(const Frustum& frustum, const Sphere& sphere);