#include "Math/OrientedBox.h"

OrientedBox::OrientedBox()
	: m_Center(Vector3f::ZERO)
	, m_Orientation(Vector3f::RIGHT, Vector3f::UP, Vector3f::FORWARD)
	, m_Radius(Vector3f::ZERO)
{
}

OrientedBox::OrientedBox(const Vector3f& center, const BasisAxes& orientation, const Vector3f& radius)
	: m_Center(center)
	, m_Orientation(orientation)
	, m_Radius(radius)
{
	assert(IsOrthonormal(m_Orientation));
}

OrientedBox::OrientedBox(u32 numPoints, const Vector3f* pFirstPoint)
{
	assert(numPoints > 0);

	Vector3f minPoint = *pFirstPoint;
	Vector3f maxPoint = *pFirstPoint;

	for (u32 pointIndex = 1; pointIndex < numPoints; ++pointIndex)
	{
		const Vector3f& point = *(pFirstPoint + pointIndex);

		minPoint = Min(minPoint, point);
		maxPoint = Max(maxPoint, point);
	}

	m_Center = 0.5f * (minPoint + maxPoint);
	
	m_Orientation.m_XAxis = Vector3f::RIGHT;
	m_Orientation.m_YAxis = Vector3f::UP;
	m_Orientation.m_ZAxis = Vector3f::FORWARD;
	assert(IsOrthonormal(m_Orientation));

	m_Radius = maxPoint - m_Center;
}
