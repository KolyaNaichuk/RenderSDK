#ifndef __MESH__
#define __MESH__

struct MeshDesc
{
	uint indexCount;
	uint startIndexLocation;
	int  baseVertexLocation;
	uint materialIndex;
};

#endif // __MESH__