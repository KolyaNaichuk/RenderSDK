#ifndef __FOUNDATION__
#define __FOUNDATION__

/* C++ and HLSL matrix storage
 * C++ matrices have row-major representation and require row-vector for multiplication on the CPU side.
 * That is, v' = v * M, v is a row-vector and M is a matrix.
 * v' = v * (WorldMatrix * ViewMatrix * ProjMatrix);
 *
 * HLSL matrices use default HLSL column-major representation and expect column-vector for multiplication in the shader.
 * That is, v' = M * v, v is a column-vector and M is a matrix.
 * v' = (ProjMatrix * ViewMatrix * WorldMatrix) * v;
 * C++ matrix should be uploaded as is and treated as reinterpreted to column-major representation (transposed) in the shader code.
 */

static const float g_PI = 3.141592654f;
static const float g_2PI = 6.283185307f;
static const float g_4PI = 12.566370616f;
static const float g_1DIVPI = 0.318309886f;
static const float g_PIDIV2 = 1.570796327f;

struct Range
{
	uint start;
	uint length;
};

struct MeshInfo
{
	uint numInstances;
	uint instanceOffset;
	uint meshType;
	uint meshTypeOffset;
	uint materialID;
	uint indexCountPerInstance;
	uint startIndexLocation;
	int  baseVertexLocation;
};

struct DrawIndexedArgs
{
	uint indexCountPerInstance;
	uint instanceCount;
	uint startIndexLocation;
	int  baseVertexLocation;
	uint startInstanceLocation;
};

struct DispatchArgs
{
	uint threadGroupCountX;
	uint threadGroupCountY;
	uint threadGroupCountZ;
};

struct DrawCommand
{
	uint instanceOffset;
	uint materialID;
	DrawIndexedArgs args;
};

struct AppData
{
	float4x4 viewMatrix;
	float4x4 viewInvMatrix;
	float4x4 projMatrix;
	float4x4 projInvMatrix;
	
	float4x4 viewProjMatrix;
	float4x4 viewProjInvMatrix;
	float4x4 prevViewProjMatrix;
	float4x4 prevViewProjInvMatrix;
	
	float4x4 notUsed1;
	float4 cameraWorldSpacePos;
	float4 cameraWorldFrustumPlanes[6];
	float cameraNearPlane;
	float cameraFarPlane;
	float2 notUsed2;
	uint2 screenSize;
	float2 rcpScreenSize;
	uint2 screenHalfSize;
	float2 rcpScreenHalfSize;
	uint2 screenQuarterSize;
	float2 rcpScreenQuarterSize;
	float4 sunWorldSpaceDir;

	float4 sunLightColor;
	uint2 screenTileSize;
	uint2 numScreenTiles;	
	float3 voxelGridWorldMinPoint;
	float notUsed3;
	float3 voxelGridWorldMaxPoint;
	float notUsed4;
	float4x4 voxelGridViewProjMatrices[3];

	float3 voxelRcpSize;
	float notUsed5;
	float notUsed6[12];
	float notUsed7[16];
	float notUsed8[16];
	float notUsed9[16];
};

#endif // __FOUNDATION__