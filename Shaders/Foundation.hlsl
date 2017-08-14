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
static const float g_TwoPI = 6.283185307f;
static const float g_RcpPI = 0.318309886f;

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
	uint materialIndex;
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