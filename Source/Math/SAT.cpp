#include "Math/SAT.h"
#include "Math/Vector3.h"

bool SAT::OverlapOnAxis(const Vector3f& axis, u32 arraySize1, const Vector3f* pointArray1, u32 arraySize2, const Vector3f* pointArray2)
{
	f32 intervalStart1, intervalEnd1;
	DetectProjectionIntervalOnAxis(intervalStart1, intervalEnd1, axis, arraySize1, pointArray1);

	f32 intervalStart2, intervalEnd2;
	DetectProjectionIntervalOnAxis(intervalStart2, intervalEnd2, axis, arraySize2, pointArray2);

	return (intervalStart2 <= intervalEnd1) && (intervalStart1 <= intervalEnd2);
}

void SAT::DetectProjectionIntervalOnAxis(f32& intervalStart, f32& intervalEnd, const Vector3f& axis, u32 arraySize, const Vector3f* pointArray)
{
	assert(IsNormalized(axis));
	assert(arraySize > 0);

	intervalStart = intervalEnd = Dot(axis, pointArray[0]);
	for (u32 index = 1; index < arraySize; ++index)
	{
		f32 pointOnInterval = Dot(axis, pointArray[index]);

		intervalStart = Min(intervalStart, pointOnInterval);
		intervalEnd = Max(intervalEnd, pointOnInterval);
	}
}
