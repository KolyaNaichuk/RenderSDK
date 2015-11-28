#include "Math/BoundingBox.h"
#include "Math/Math.h"

BoundingBox::BoundingBox(const Vector3f& center, const Vector3f& radius)
    : m_Center(center)
    , m_Radius(radius)
{
}

BoundingBox::BoundingBox(u32 numPoints, const Vector3f* pFirstPoint)
{
	Vector3f minPoint = *pFirstPoint;
	Vector3f maxPoint = *pFirstPoint;

	for (u32 pointIndex = 1; pointIndex < numPoints; ++pointIndex)
	{
		const Vector3f& point = *(pFirstPoint + pointIndex);

		minPoint = Min(minPoint, point);
		maxPoint = Max(maxPoint, point);
	}

	m_Center = 0.5f * (minPoint + maxPoint);
	m_Radius = maxPoint - m_Center;
}

BoundingBox::BoundingBox(const BoundingBox& box1, const BoundingBox& box2)
{
    Vector3f minPoint = Min(box1.m_Center - box1.m_Radius, box2.m_Center - box2.m_Radius);
    Vector3f maxPoint = Max(box1.m_Center + box1.m_Radius, box2.m_Center + box2.m_Radius);

    m_Center = 0.5f * (minPoint + maxPoint);
    m_Radius = maxPoint - m_Center;
}
