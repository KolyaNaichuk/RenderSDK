#pragma once

struct AxisAlignedBox;
struct Sphere;

bool Overlap(const AxisAlignedBox& box1, const AxisAlignedBox& box2);
bool Overlap(const Sphere& sphere1, const Sphere& sphere2);