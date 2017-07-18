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

struct MeshDesc
{
	uint indexCount;
	uint startIndexLocation;
	int  baseVertexLocation;
	uint materialIndex;
};

struct DrawIndexedArgs
{
	uint indexCountPerInstance;
	uint instanceCount;
	uint startIndexLocation;
	int  baseVertexLocation;
	uint startInstanceLocation;
};

struct DrawMeshCommand
{
	uint root32BitConstant;
	DrawIndexedArgs drawArgs;
};

#endif // __FOUNDATION__