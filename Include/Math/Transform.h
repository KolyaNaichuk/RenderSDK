#pragma once

#include "Math/Matrix4f.h"
#include "Math/Vector4f.h"

struct Vector3f;

namespace Transform
{
    const Matrix4f Translation(f32 xOffset, f32 yOffset, f32 zOffset);
    const Matrix4f Scaling(f32 xScale, f32 yScale, f32 zScale);
    const Matrix4f RotationX(f32 angleInRadians);
    const Matrix4f RotationY(f32 angleInRadians);
    const Matrix4f RotationZ(f32 angleInRadians);
    const Matrix4f LookAt(const Vector3f& eyePos, const Vector3f& lookAtPos, const Vector3f& upDir);
	const Matrix4f OrthoOffCenterProj(f32 leftX, f32 rightX, f32 bottomY, f32 topY, f32 nearZ, f32 farZ);
	const Matrix4f OrthoProj(f32 width, f32 height, f32 nearZ, f32 farZ);
    const Matrix4f PerspectiveProj(f32 nearWidth, f32 nearHeight, f32 nearZ, f32 farZ);
    const Matrix4f PerspectiveFovProj(f32 fovYInRadians, f32 aspectRatio, f32 nearZ, f32 farZ);

    const Vector4f TransformVector(const Vector4f& vec, const Matrix4f& matrix);
    const Vector4f TransformNormal(const Vector4f& vec, const Matrix4f& matrix);
};
