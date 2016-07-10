#ifndef __OVERLAP_TEST__
#define __OVERLAP_TEST__

struct AABB
{
	float3 center;
	float3 radius;
};

struct Sphere
{
	float3 center;
	float radius;
};

bool TestAABBAgainstPlane(float4 plane, AABB aabb)
{
	float maxRadiusProj = dot(aabb.radius, abs(plane.xyz));
	float signedDist = dot(plane, float4(aabb.center, 1.0f));
	
	bool fullyInsideBackHalfSpace = (signedDist + maxRadiusProj) < 0.0f;
	return !fullyInsideBackHalfSpace;
}

bool TestSphereAgainstPlane(float4 plane, Sphere sphere)
{
	float signedDist = dot(plane, float4(sphere.center, 1.0f));

	bool fullyInsideBackHalfSpace = (signedDist + sphere.radius) < 0.0f;
	return !fullyInsideBackHalfSpace;
}

bool TestPointAgainstPlane(float4 plane, float3 pt)
{
	float signedDist = dot(plane, float4(pt, 1.0f));

	bool fullyInsideBackHalfSpace = signedDist < 0.0f;
	return !fullyInsideBackHalfSpace;
}

bool TestAABBAgainstFrustum(float4 frustumPlanes[6], AABB aabb)
{
	bool insideOrOverlap = TestAABBAgainstPlane(frustumPlanes[0], aabb) &&
		TestAABBAgainstPlane(frustumPlanes[1], aabb) &&
		TestAABBAgainstPlane(frustumPlanes[2], aabb) &&
		TestAABBAgainstPlane(frustumPlanes[3], aabb) &&
		TestAABBAgainstPlane(frustumPlanes[4], aabb) &&
		TestAABBAgainstPlane(frustumPlanes[5], aabb);

	return insideOrOverlap;
}

bool TestSphereAgainstFrustum(float4 frustumPlanes[6], Sphere sphere)
{
	bool insideOrOverlap = TestSphereAgainstPlane(frustumPlanes[0], sphere) &&
		TestSphereAgainstPlane(frustumPlanes[1], sphere) &&
		TestSphereAgainstPlane(frustumPlanes[2], sphere) &&
		TestSphereAgainstPlane(frustumPlanes[3], sphere) &&
		TestSphereAgainstPlane(frustumPlanes[4], sphere) &&
		TestSphereAgainstPlane(frustumPlanes[5], sphere);

	return insideOrOverlap;
}

bool TestSphereAgainstAABB(AABB aabb, Sphere sphere)
{
	float3 delta = max(0.0f, abs(aabb.center - sphere.center) - sphere.radius);
	float sqDist = dot(delta, delta);
	bool insideOrOverlap = sqDist <= (sphere.radius * sphere.radius);
	
	return insideOrOverlap;
}

#endif // __OVERLAP_TEST__