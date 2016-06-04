#ifndef __OVERLAP_TEST__
#define __OVERLAP_TEST__

struct AABB
{
	float3 center;
	float3 radius;
};

bool TestAABBAgainstPlane(float4 plane, AABB aabb)
{
	float maxRadiusProj = dot(aabb.radius, abs(plane.xyz));
	float signedDist = dot(plane, float4(aabb.center, 1.0f));
	
	bool fullyInsideBackHalfSpace = (signedDist + maxRadiusProj) < 0.0f;
	return !fullyInsideBackHalfSpace;
}

bool TestSphereAgainstPlane(float4 plane, float3 sphereCenter, float sphereRadius)
{
	float signedDist = dot(plane, float4(sphereCenter, 1.0f));

	bool fullyInsideBackHalfSpace = (signedDist + sphereRadius) < 0.0f;
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

bool TestSphereAgainstFrustum(float4 frustumPlanes[6], float3 sphereCenter, float sphereRadius)
{
	bool insideOrOverlap = TestSphereAgainstPlane(frustumPlanes[0], sphereCenter, sphereRadius) &&
		TestSphereAgainstPlane(frustumPlanes[1], sphereCenter, sphereRadius) &&
		TestSphereAgainstPlane(frustumPlanes[2], sphereCenter, sphereRadius) &&
		TestSphereAgainstPlane(frustumPlanes[3], sphereCenter, sphereRadius) &&
		TestSphereAgainstPlane(frustumPlanes[4], sphereCenter, sphereRadius) &&
		TestSphereAgainstPlane(frustumPlanes[5], sphereCenter, sphereRadius);

	return insideOrOverlap;
}

bool TestSphereAgainstAABB(AABB aabb, float3 sphereCenter, float sphereRadius)
{
	float3 delta = max(0.0f, abs(aabb.center - sphereCenter) - sphereRadius);
	float sqDist = dot(delta, delta);
	bool insideOrOverlap = sqDist <= (sphereRadius * sphereRadius);
	
	return insideOrOverlap;
}

#endif // __OVERLAP_TEST__