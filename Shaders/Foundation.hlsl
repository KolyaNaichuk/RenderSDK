#ifndef __FOUNDATION__
#define __FOUNDATION__

static const float g_PI = 3.141592654f;
static const float g_TwoPI = 6.283185307f;
static const float g_RcpPI = 0.318309886f;

struct Range
{
	uint start;
	uint length;
};

struct DrawIndexedArgs
{
	uint indexCountPerInstance;
	uint instanceCount;
	uint startIndexLocation;
	int  baseVertexLocation;
	uint startInstanceLocation;
};

struct AppData
{
	matrix viewProjMatrix;
	matrix viewProjInvMatrix;
	matrix prevViewProjMatrix;
	matrix prevViewProjInvMatrix;
	float4 cameraWorldFrustumPlanes[6];
	uint2 screenSize;
	float2 rcpScreenSize;
	uint2 screenHalfSize;
	float2 rcpScreenHalfSize;
	uint2 screenQuarterSize;
	float2 rcpScreenQuarterSize;
	float4 notUsed[7];
};

#endif // __FOUNDATION__