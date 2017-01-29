#ifndef __BOUNDING_VOLUMES__
#define __BOUNDING_VOLUMES__

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

#endif // __BOUNDING_VOLUMES__