#include "RayTracingUtils.hlsl"

struct RayPayload
{
	float4 color;
};

struct Vertex
{
	float3 worldPos;
	float4 color;
};

struct AppData
{
	float3 cameraWorldPos;
	float rayMinExtent;
	float3 cameraWorldAxisX;
	float rayMaxExtent;
	float3 cameraWorldAxisY;
	float notUsed1;
	float3 cameraWorldAxisZ;
	float notUsed2;
	float4 backgroundColor;
	float notUsed3[12];
	float notUsed4[16];
	float notUsed5[16];
};

ConstantBuffer<AppData> g_AppDataBuffer : register(b0);

RaytracingAccelerationStructure g_TopLevelAccelStruct : register(t0);
StructuredBuffer<Vertex> g_VertexBuffer : register(t1);
Buffer<uint> g_IndexBuffer : register(t2);

RWTexture2D<float4> g_OutputTexture : register(u0);

[shader("raygeneration")]
void RayGeneration()
{
	uint2 pixelPos = DispatchRaysIndex().xy;
	uint2 numPixels = DispatchRaysDimensions().xy;

	float2 localPos = (float2(pixelPos) + 0.5f) / float2(numPixels); // [0..1]
	localPos = float2(2.0f, -2.0f) * localPos + float2(-1.0f, 1.0f); // [-1..1]

	float3 localDir = float3(localPos.xy, 1.0f);
	float3 worldDir = localDir.x * g_AppDataBuffer.cameraWorldAxisX + localDir.y * g_AppDataBuffer.cameraWorldAxisY + localDir.z * g_AppDataBuffer.cameraWorldAxisZ;
	worldDir = normalize(worldDir);

	RayDesc ray;
	ray.Origin = g_AppDataBuffer.cameraWorldPos;
	ray.Direction = worldDir;
	ray.TMin = g_AppDataBuffer.rayMinExtent;
	ray.TMax = g_AppDataBuffer.rayMaxExtent;

	RayPayload payload;
	TraceRay(g_TopLevelAccelStruct, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF/*instanceInclusionMask*/,
		0/*hitGroupIndex*/, 1/*numHitGroups*/, 0/*missShaderIndex*/, ray, payload);
	
	g_OutputTexture[pixelPos] = payload.color;
}

[shader("miss")]
void RayMiss(inout RayPayload payload)
{
	payload.color = g_AppDataBuffer.backgroundColor;
}

[shader("closesthit")]
void RayClosestHit(inout RayPayload payload, BuiltInTriangleIntersectionAttributes attribs)
{
	const uint firstIndexOffset = 3 * PrimitiveIndex();
	
	uint index1 = g_IndexBuffer[firstIndexOffset];
	uint index2 = g_IndexBuffer[firstIndexOffset + 1];
	uint index3 = g_IndexBuffer[firstIndexOffset + 2];

	float4 colors[3];
	colors[0] = g_VertexBuffer[index1].color;
	colors[1] = g_VertexBuffer[index2].color;
	colors[2] = g_VertexBuffer[index3].color;

	payload.color = InterpolateTriangleAttributes(colors, attribs.barycentrics);
}