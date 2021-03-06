#include "Math/OverlapTest.h"
#include "Math/AxisAlignedBox.h"
#include "Math/Frustum.h"
#include "Math/Sphere.h"
#include "Math/Plane.h"
#include "Math/Math.h"
#include "Math/SAT.h"

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

bool TestAABBAgainstPlane(const Plane& plane, const AxisAlignedBox& box)
{
	f32 maxRadiusProj = Dot(box.m_Radius, Abs(plane.m_Normal));
	f32 signedDist = SignedDistanceToPoint(plane, box.m_Center);

	bool fullyInsideBackHalfSpace = (signedDist + maxRadiusProj) < 0.0f;
	return !fullyInsideBackHalfSpace;
}

bool TestSphereAgainstPlane(const Plane& plane, const Sphere& sphere)
{
	f32 signedDist = SignedDistanceToPoint(plane, sphere.m_Center);
	
	bool fullyInsideBackHalfSpace = (signedDist + sphere.m_Radius) < 0.0f;
	return !fullyInsideBackHalfSpace;
}

bool TestPointAgainstPlane(const Plane& plane, const Vector3f& point)
{
	float signedDist = SignedDistanceToPoint(plane, point);

	bool fullyInsideBackHalfSpace = signedDist < 0.0f;
	return !fullyInsideBackHalfSpace;
}

bool TestFrustumAgainstFrustum(const Frustum& frustum1, const Frustum& frustum2)
{
	return SAT::OverlapOnAxis(frustum1.m_Planes[Frustum::FarPlane].m_Normal, Frustum::NumCorners, frustum1.m_Corners, Frustum::NumCorners, frustum2.m_Corners) && 
		SAT::OverlapOnAxis(frustum1.m_Planes[Frustum::LeftPlane].m_Normal, Frustum::NumCorners, frustum1.m_Corners, Frustum::NumCorners, frustum2.m_Corners) &&
		SAT::OverlapOnAxis(frustum1.m_Planes[Frustum::RightPlane].m_Normal, Frustum::NumCorners, frustum1.m_Corners, Frustum::NumCorners, frustum2.m_Corners) &&
		SAT::OverlapOnAxis(frustum1.m_Planes[Frustum::TopPlane].m_Normal, Frustum::NumCorners, frustum1.m_Corners, Frustum::NumCorners, frustum2.m_Corners) &&
		SAT::OverlapOnAxis(frustum1.m_Planes[Frustum::BottomPlane].m_Normal, Frustum::NumCorners, frustum1.m_Corners, Frustum::NumCorners, frustum2.m_Corners) &&
		SAT::OverlapOnAxis(frustum2.m_Planes[Frustum::FarPlane].m_Normal, Frustum::NumCorners, frustum1.m_Corners, Frustum::NumCorners, frustum2.m_Corners) &&
		SAT::OverlapOnAxis(frustum2.m_Planes[Frustum::LeftPlane].m_Normal, Frustum::NumCorners, frustum1.m_Corners, Frustum::NumCorners, frustum2.m_Corners) &&
		SAT::OverlapOnAxis(frustum2.m_Planes[Frustum::RightPlane].m_Normal, Frustum::NumCorners, frustum1.m_Corners, Frustum::NumCorners, frustum2.m_Corners) &&
		SAT::OverlapOnAxis(frustum2.m_Planes[Frustum::TopPlane].m_Normal, Frustum::NumCorners, frustum1.m_Corners, Frustum::NumCorners, frustum2.m_Corners) &&
		SAT::OverlapOnAxis(frustum2.m_Planes[Frustum::BottomPlane].m_Normal, Frustum::NumCorners, frustum1.m_Corners, Frustum::NumCorners, frustum2.m_Corners);
}

bool TestAABBAgainstFrustum(const Frustum& frustum, const AxisAlignedBox& box)
{
	bool insideOrOverlap = TestAABBAgainstPlane(frustum.m_Planes[Frustum::NearPlane], box) &&
		TestAABBAgainstPlane(frustum.m_Planes[Frustum::FarPlane], box) &&
		TestAABBAgainstPlane(frustum.m_Planes[Frustum::LeftPlane], box) &&
		TestAABBAgainstPlane(frustum.m_Planes[Frustum::RightPlane], box) &&
		TestAABBAgainstPlane(frustum.m_Planes[Frustum::TopPlane], box) &&
		TestAABBAgainstPlane(frustum.m_Planes[Frustum::BottomPlane], box);

	return insideOrOverlap;
}

bool TestSphereAgainstFrustum(const Frustum& frustum, const Sphere& sphere)
{
	bool insideOrOverlap = TestSphereAgainstPlane(frustum.m_Planes[Frustum::NearPlane], sphere) &&
		TestSphereAgainstPlane(frustum.m_Planes[Frustum::FarPlane], sphere) &&
		TestSphereAgainstPlane(frustum.m_Planes[Frustum::LeftPlane], sphere) &&
		TestSphereAgainstPlane(frustum.m_Planes[Frustum::RightPlane], sphere) &&
		TestSphereAgainstPlane(frustum.m_Planes[Frustum::TopPlane], sphere) &&
		TestSphereAgainstPlane(frustum.m_Planes[Frustum::BottomPlane], sphere);

	return insideOrOverlap;
}

