#ifndef __LIGHTING__
#define __LIGHTING__

struct PointLight
{
	float3 worldSpacePos;
};

struct SpotLight
{
	float3 worldSpacePos;
};

#endif // __LIGHTING__