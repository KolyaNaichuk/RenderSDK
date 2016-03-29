#include "Math/OverlapTest.h"
#include "Math/AxisAlignedBox.h"
#include "Math/Sphere.h"
#include "Math/Math.h"

bool Overlap(const AxisAlignedBox& box1, const AxisAlignedBox& box2)
{
	if (Abs(box1.m_Center.m_X - box2.m_Center.m_X) > (box1.m_Radius.m_X + box2.m_Radius.m_X))
		return false;

	if (Abs(box1.m_Center.m_Y - box2.m_Center.m_Y) > (box1.m_Radius.m_Y + box2.m_Radius.m_Y))
		return false;
	
	if (Abs(box1.m_Center.m_Z - box2.m_Center.m_Z) > (box1.m_Radius.m_Z + box2.m_Radius.m_Z))
		return false;
	
	return true;
}

bool Overlap(const Sphere& sphere1, const Sphere& sphere2)
{
	f32 sqLength = LengthSquared(sphere1.m_Center - sphere2.m_Center);
	f32 radiusSum = sphere1.m_Radius + sphere2.m_Radius;
	
	return (sqLength <= Sqr(radiusSum));
}
