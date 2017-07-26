#include "Math/AxisAlignedBox.h"
#include "Math/Math.h"

AxisAlignedBox::AxisAlignedBox()
	: AxisAlignedBox(Vector3f::ZERO, Vector3f::ZERO)
{
}

AxisAlignedBox::AxisAlignedBox(const Vector3f& center, const Vector3f& radius)
    : m_Center(center)
    , m_Radius(radius)
{
}

AxisAlignedBox::AxisAlignedBox(u32 numPoints, const Vector3f* pFirstPoint)
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

AxisAlignedBox::AxisAlignedBox(const AxisAlignedBox& box1, const AxisAlignedBox& box2)
{
    Vector3f minPoint = Min(box1.m_Center - box1.m_Radius, box2.m_Center - box2.m_Radius);
    Vector3f maxPoint = Max(box1.m_Center + box1.m_Radius, box2.m_Center + box2.m_Radius);

    m_Center = 0.5f * (minPoint + maxPoint);
    m_Radius = maxPoint - m_Center;
}
