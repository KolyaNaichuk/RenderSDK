#ifndef __FOUNDATION__
#define __FOUNDATION__

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

struct Material
{
	float4 ambientColor;
	float4 diffuseColor;
	float4 specularColor;
	float  specularPower;
	float4 emissiveColor;
};

#endif // __FOUNDATION__