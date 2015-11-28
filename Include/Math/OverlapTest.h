#pragma once

struct BoundingBox;
struct BoundingSphere;

bool Overlap(const BoundingBox& box1, const BoundingBox& box2);
bool Overlap(const BoundingSphere& sphere1, const BoundingSphere& sphere2);