#pragma once

#include "Math/Math.h"

struct Vector3f;

namespace SAT
{
	bool OverlapOnAxis(const Vector3f& axis, u32 arraySize1, const Vector3f* pointArray1, u32 arraySize2, const Vector3f* pointArray2);
	void DetectProjectionIntervalOnAxis(f32& intervalStart, f32& intervalEnd, const Vector3f& axis, u32 arraySize, const Vector3f* pointArray);
};