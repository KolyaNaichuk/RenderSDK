#include "VoxelGrid.hlsl"
#include "BoundingVolumes.hlsl"
#include "Lighting.hlsl"
#include "SphericalHarmonics.hlsl"
#include "Foundation.hlsl"

cbuffer GridConfigBuffer : register(b0)
{
	GridConfig g_GridConfig;
}

StructuredBuffer<Voxel> g_GridBuffer : register(t0);

#if ENABLE_POINT_LIGHTS == 1
StructuredBuffer<Sphere> g_PointLightBoundsBuffer : register(t1);
StructuredBuffer<PointLightProps> g_PointLightPropsBuffer : register(t2);
Buffer<uint> g_NumPointLightsBuffer : register(t3);
Buffer<uint> g_PointLightIndexBuffer : register(t4);
#endif // ENABLE_POINT_LIGHTS

RWTexture3D<float4> g_IntensityRCoeffsTexture : register(u0);
RWTexture3D<float4> g_IntensityGCoeffsTexture : register(u1);
RWTexture3D<float4> g_IntensityBCoeffsTexture : register(u2);

RWTexture3D<float4> g_AccumIntensityRCoeffsTexture : register(u3);
RWTexture3D<float4> g_AccumIntensityGCoeffsTexture : register(u4);
RWTexture3D<float4> g_AccumIntensityBCoeffsTexture : register(u5);

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, NUM_THREADS_Z)]
void Main(int3 gridCell : SV_DispatchThreadID)
{
	SHSpectralCoeffs totalIntensityCoeffs;
	totalIntensityCoeffs.r = float4(0.0f, 0.0f, 0.0f, 0.0f);
	totalIntensityCoeffs.g = float4(0.0f, 0.0f, 0.0f, 0.0f);
	totalIntensityCoeffs.b = float4(0.0f, 0.0f, 0.0f, 0.0f);

	const int cellIndex = ComputeGridCellIndex(g_GridConfig, gridCell);
	if (g_GridBuffer[cellIndex].numOccluders > 0.0f)
	{
		float3 diffuseAlbedo = g_GridBuffer[cellIndex].diffuseColor;
		float3 worldSpaceNormal = g_GridBuffer[cellIndex].worldSpaceNormal;

#if ENABLE_POINT_LIGHTS == 1
		for (uint index = 0; index < g_NumPointLightsBuffer[0]; ++index)
		{
			uint lightIndex = g_PointLightIndexBuffer[index];

			float3 worldSpaceLightPos = g_PointLightBoundsBuffer[lightIndex].center;
			float lightAttenEndRange = g_PointLightBoundsBuffer[lightIndex].radius;

			float3 worldSpaceCellPos = ComputeWorldSpacePosition(g_GridConfig, gridCell);
			float3 worldSpaceDirToLight = worldSpaceLightPos - worldSpaceCellPos;

			float surfaceArea = 1024.0f;// Kolya. Hardcoding for now
			float distToLightSquared = dot(worldSpaceDirToLight, worldSpaceDirToLight);
			float solidAngle = 1.0f;// surfaceArea / distToLightSquared;
						
			float3 lightIntensity = g_PointLightPropsBuffer[lightIndex].color;
			
			float NdotL = saturate(dot(worldSpaceNormal, normalize(worldSpaceDirToLight)));
			float3 reflectedFlux = diffuseAlbedo * lightIntensity * (solidAngle * NdotL);
						
			float4 cosineCoeffs = SHProjectClampedCosine(worldSpaceNormal);
			SHSpectralCoeffs intensityCoeffs;
			intensityCoeffs.r = reflectedFlux.r * cosineCoeffs;
			intensityCoeffs.g = reflectedFlux.g * cosineCoeffs;
			intensityCoeffs.b = reflectedFlux.b * cosineCoeffs;

			totalIntensityCoeffs.r += intensityCoeffs.r;
			totalIntensityCoeffs.g += intensityCoeffs.g;
			totalIntensityCoeffs.b += intensityCoeffs.b;
		}
#endif // ENABLE_POINT_LIGHTS
	}

	const uint3 texturePos = ComputeTexturePosition(g_GridConfig, gridCell);

	g_IntensityRCoeffsTexture[texturePos] = totalIntensityCoeffs.r;
	g_IntensityGCoeffsTexture[texturePos] = totalIntensityCoeffs.g;
	g_IntensityBCoeffsTexture[texturePos] = totalIntensityCoeffs.b;

	g_AccumIntensityRCoeffsTexture[texturePos] = totalIntensityCoeffs.r;
	g_AccumIntensityGCoeffsTexture[texturePos] = totalIntensityCoeffs.g;
	g_AccumIntensityBCoeffsTexture[texturePos] = totalIntensityCoeffs.b;
}